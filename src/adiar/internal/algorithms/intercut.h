#ifndef ADIAR_INTERNAL_ALGORITHMS_INTERCUT_H
#define ADIAR_INTERNAL_ALGORITHMS_INTERCUT_H

#include <adiar/data.h>

#include <adiar/file.h>
#include <adiar/file_stream.h>
#include <adiar/file_writer.h>

#include <adiar/internal/assert.h>
#include <adiar/internal/cnl.h>
#include <adiar/internal/cut.h>

#include <adiar/internal/data_structures/levelized_priority_queue.h>

namespace adiar
{
  //////////////////////////////////////////////////////////////////////////////
  /// Struct to hold statistics
  extern stats_t::intercut_t stats_intercut;

  //////////////////////////////////////////////////////////////////////////////
  // Priority queue

  struct intercut_req : arc
  {
    label_t level = MAX_LABEL + 1u;
  };

  struct intercut_req_label
  {
    static label_t label_of(const intercut_req &r)
    {
      return r.level;
    }
  };

  struct intercut_req_lt
  {
    bool operator()(const intercut_req &a, const intercut_req &b)
    {
      return a.level < b.level
        || (a.level == b.level && a.target < b.target)
#ifndef NDEBUG
        || (a.level == b.level && a.target == b.target && a.source < b.source)
#endif
           ;
    }
  };

  template<size_t LOOK_AHEAD, memory::memory_mode mem_mode>
  using intercut_priority_queue_t =
    levelized_label_priority_queue<intercut_req, intercut_req_label, intercut_req_lt,
                                   LOOK_AHEAD,
                                   mem_mode,
                                   2u,
                                   0u>;

  //////////////////////////////////////////////////////////////////////////////

  struct intercut_rec_output
  {
    ptr_t low;
    ptr_t high;
  };

  struct intercut_rec_skipto
  {
    ptr_t tgt;
  };

  typedef std::variant<intercut_rec_output, intercut_rec_skipto> intercut_rec;

  //////////////////////////////////////////////////////////////////////////////
  // Helper functions
  template<typename intercut_policy>
  bool cut_terminal(const label_t curr_level, const label_t cut_level, const bool terminal_value)
  {
    return curr_level < cut_level
      && cut_level <= MAX_LABEL
      && (!terminal_value || intercut_policy::cut_true_terminal)
      && (terminal_value || intercut_policy::cut_false_terminal);
  }

  template<typename intercut_policy>
  class intercut_out__pq
  {
  public:
    static constexpr bool ignore_nil = false;

    template<typename pq_t>
    static inline void forward(arc_writer &aw,
                               pq_t &pq,
                               const ptr_t source, const ptr_t target,
                               const label_t curr_level, const label_t next_cut)
    {
      const label_t target_level = is_node(target) ? label_of(target) : MAX_LABEL+1;
      if (is_terminal(target) && !cut_terminal<intercut_policy>(curr_level, next_cut, value_of(target))) {
        aw.unsafe_push_terminal({ source, target });
        return;
      }
      pq.push({ source, target, std::min(target_level, next_cut) });
    }
  };

  class intercut_out__writer
  {
  public:
    static constexpr bool ignore_nil = true;

    template<typename pq_t>
    static inline void forward(arc_writer &aw,
                               pq_t &/*pq*/,
                               const ptr_t source, const ptr_t target,
                               const label_t /*curr_level*/, const label_t /*next_cut*/)
    {
      aw.unsafe_push_node({ source, target });
    }
  };

  template<typename intercut_policy, typename in_policy, typename pq_t>
  inline void intercut_in__pq(arc_writer &aw,
                                pq_t &pq,
                                const label_t out_label,
                                const ptr_t pq_target,
                                const ptr_t out_target,
                                const label_t l)
  {
    adiar_debug(out_label <= label_of(out_target),
                "should forward/output a node on this level or ahead.");

    while (pq.can_pull()
           && pq.top().level == out_label
           && pq.top().target == pq_target) {
      const intercut_req parent = pq.pull();

      if (in_policy::ignore_nil && is_nil(parent.source)) { continue; }
      in_policy::forward(aw, pq, parent.source, out_target, out_label, l);
    }
  }

  template<typename intercut_policy>
  label_file create_dd_labels(const typename intercut_policy::reduced_t &dd)
  {
    label_file labels;
    label_writer writer(labels);

    level_info_stream<node_t> info_stream(dd);

    while(info_stream.can_pull()) {
      writer << label_of(info_stream.pull());
    }

    return labels;
  }

  template<typename intercut_policy, typename pq_t>
  typename intercut_policy::unreduced_t __intercut (const typename intercut_policy::reduced_t &dd,
                                                    const label_file &labels,
                                                    const size_t pq_memory,
                                                    const size_t max_pq_size)
  {
    node_stream<> in_nodes(dd);
    node_t n = in_nodes.pull();

    if (is_terminal(n)) {
      return intercut_policy::on_terminal_input(value_of(n), dd, labels);
    }

    label_stream<> ls(labels);
    label_t l = ls.pull();

    arc_file out_arcs;
    arc_writer aw(out_arcs);

    label_file dd_labels = create_dd_labels<intercut_policy>(dd);
    pq_t intercut_pq({dd_labels, labels}, pq_memory, max_pq_size, stats_intercut.lpq);

    // Add request for root in the queue
    label_t out_label = std::min(l, label_of(n));
    intercut_pq.push({ NIL, n.uid, out_label });
    id_t out_id = 0;

    size_t max_1level_cut = 0;

    // Process nodes of the decision diagram in topological order
    while (!intercut_pq.empty()) {
      if (out_id > 0) { aw.unsafe_push(create_level_info(out_label, out_id)); }

      intercut_pq.setup_next_level();
      out_label = intercut_pq.current_level();

      max_1level_cut = std::max(max_1level_cut, intercut_pq.size());

      out_id = 0;

      const bool hit_level = out_label == l;

      // Forward to next label to cut on after this level
      while (ls.can_pull() && l <= out_label) {
        l = ls.pull();
      }

      if(!ls.can_pull() && l <= out_label) {
        l = MAX_LABEL + 1;
      }

      // Resolve requests that end at the cut for this level
      while (intercut_pq.can_pull() && label_of(intercut_pq.peek().target) == intercut_pq.peek().level) {
        while (n.uid < intercut_pq.top().target) {
          n = in_nodes.pull();
        }

        adiar_debug(n.uid == intercut_pq.top().target, "should always find desired node");

        const intercut_rec r = hit_level
          ? intercut_policy::hit_existing(n)
          : intercut_policy::miss_existing(n);

        if (intercut_policy::may_skip && std::holds_alternative<intercut_rec_skipto>(r)) {
          const intercut_rec_skipto rs = std::get<intercut_rec_skipto>(r);

          if (is_terminal(rs.tgt)
              && is_nil(intercut_pq.top().source)
              && !cut_terminal<intercut_policy>(out_label, l, value_of(rs.tgt))) {
            return intercut_policy::terminal(value_of(rs.tgt));
          }
          // TODO: The 'is_terminal(rs.tgt) && cut_terminal(...)' case can be handled even
          //       better with 'intercut_policy::on_terminal_input' but where the
          //       label file are only of the remaining labels.

          intercut_in__pq<intercut_policy, intercut_out__pq<intercut_policy>>
            (aw, intercut_pq, out_label, n.uid, rs.tgt, l);
        } else {
          const intercut_rec_output ro = std::get<intercut_rec_output>(r);
          const uid_t out_uid = create_node_uid(out_label, out_id++);

          intercut_out__pq<intercut_policy>::forward
            (aw, intercut_pq, out_uid, ro.low, out_label, l);

          intercut_out__pq<intercut_policy>::forward
            (aw, intercut_pq, flag(out_uid), ro.high, out_label, l);

          intercut_in__pq<intercut_policy, intercut_out__writer>
            (aw, intercut_pq, out_label, n.uid, out_uid, l);
        }
      }

      // Resolve requests that end after the cut for this level
      while(intercut_pq.can_pull()) {
        adiar_invariant(out_label <= l,
                          "the last iteration in this case is for the very last label to cut on");

        const intercut_req request = intercut_pq.top();
        const intercut_rec_output ro = intercut_policy::hit_cut(request.target);
        const uid_t out_uid = create_node_uid(out_label, out_id++);

        intercut_out__pq<intercut_policy>::forward
          (aw, intercut_pq, out_uid, ro.low, out_label, l);

        intercut_out__pq<intercut_policy>::forward
          (aw, intercut_pq, flag(out_uid), ro.high, out_label, l);

        intercut_in__pq<intercut_policy, intercut_out__writer>
          (aw, intercut_pq, out_label, request.target, out_uid, l);
      }
    }

    // Push the level of the very last iteration
    if (out_id > 0) {
      aw.unsafe_push(create_level_info(out_label, out_id));
    }

    out_arcs->max_1level_cut = max_1level_cut;
    return out_arcs;
  }

  template<typename intercut_policy>
  size_t __intercut_2level_upper_bound(const typename intercut_policy::reduced_t &dd)
  {
    const cut_type ct = cut_type_with(intercut_policy::cut_false_terminal,
                                      intercut_policy::cut_true_terminal);
    const safe_size_t max_1level_cut = dd.max_1level_cut(ct);

    return to_size((3 * intercut_policy::mult_factor * max_1level_cut) / 2 + 2);
  }

  template<typename intercut_policy>
  typename intercut_policy::unreduced_t intercut(const typename intercut_policy::reduced_t &dd,
                                                 const label_file &labels)
  {
    if (labels.size() == 0) {
      return intercut_policy::on_empty_labels(dd);
    }

    // Compute amount of memory available for auxiliary data structures after
    // having opened all streams.
    //
    // We then may derive an upper bound on the size of auxiliary data
    // structures and check whether we can run them with a faster internal
    // memory variant.
    const tpie::memory_size_type aux_available_memory = memory::available()
      // Input stream
      - node_stream<>::memory_usage()
      // Output stream
      - arc_writer::memory_usage();

    const size_t pq_memory = aux_available_memory;

    const size_t pq_memory_fits =
      intercut_priority_queue_t<ADIAR_LPQ_LOOKAHEAD, memory::memory_mode::INTERNAL>::memory_fits(pq_memory);

    const bool internal_only = memory::mode == memory::INTERNAL;
    const bool external_only = memory::mode == memory::EXTERNAL;

    const size_t pq_bound = __intercut_2level_upper_bound<intercut_policy>(dd);

    const size_t max_pq_size = internal_only ? std::min(pq_memory_fits, pq_bound) : pq_bound;

    if(!external_only && max_pq_size <= no_lookahead_bound()) {
#ifdef ADIAR_STATS
      stats_intercut.lpq.unbucketed++;
#endif
      return __intercut<intercut_policy,
                        intercut_priority_queue_t<0, memory::INTERNAL>>
        (dd, labels, pq_memory, max_pq_size);
    } else if(!external_only && max_pq_size <= pq_memory_fits) {
#ifdef ADIAR_STATS
      stats_intercut.lpq.internal++;
#endif
      return __intercut<intercut_policy,
                        intercut_priority_queue_t<ADIAR_LPQ_LOOKAHEAD, memory::INTERNAL>>
        (dd, labels, pq_memory, max_pq_size);
    } else {
#ifdef ADIAR_STATS
      stats_intercut.lpq.external++;
#endif
      return __intercut<intercut_policy,
                        intercut_priority_queue_t<ADIAR_LPQ_LOOKAHEAD, memory::EXTERNAL>>
        (dd, labels, pq_memory, max_pq_size);
    }
  }
}

#endif // ADIAR_INTERNAL_ALGORITHMS_INTERCUT_H