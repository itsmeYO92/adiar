#ifndef ADIAR_INTERNAL_DATA_TYPES_LEVEL_INFO_H
#define ADIAR_INTERNAL_DATA_TYPES_LEVEL_INFO_H

#include <adiar/internal/data_types/ptr.h>

namespace adiar::internal
{
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Meta information on a single level in a decision diagram.
  ///
  /// \details Several of our algorithms and data structures exploit some meta
  ///          information to improve their performance.
  //////////////////////////////////////////////////////////////////////////////
  struct level_info
  {
    ptr_uint64::label_t label;
    size_t width;
  };

  //////////////////////////////////////////////////////////////////////////////
  /// \copydoc
  //////////////////////////////////////////////////////////////////////////////
  typedef level_info level_info_t;

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Combine all the meta information for a single level
  //////////////////////////////////////////////////////////////////////////////
  inline level_info_t create_level_info(ptr_uint64::label_t label, size_t level_width)
  {
    return { label, level_width };
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Get the variable label of a specific level.
  //////////////////////////////////////////////////////////////////////////////
  inline ptr_uint64::label_t label_of(const level_info_t &m)
  {
    return m.label;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Get the number of nodes on a specific level.
  //////////////////////////////////////////////////////////////////////////////
  inline size_t width_of(const level_info_t &m)
  {
    return m.width;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// \internal This is only due to some file_stream requires the existence of a
  /// ! operator.
  //////////////////////////////////////////////////////////////////////////////
  inline level_info operator! (const level_info &m)
  {
    return m;
  }

  inline bool operator== (const level_info &a, const level_info &b)
  {
    return a.label == b.label && a.width == b.width;
  }

  inline bool operator!= (const level_info &a, const level_info &b)
  {
    return !(a==b);
  }
}

#endif // ADIAR_INTERNAL_DATA_TYPES_LEVEL_INFO_H