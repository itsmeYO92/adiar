#include "../../test.h"

go_bandit([]() {
  describe("adiar/zdd/contains.cpp", [&]() {
    shared_levelized_file<zdd::node_t> zdd_F;
    shared_levelized_file<zdd::node_t> zdd_T;

    { // Garbage collect writers to free write-lock
      node_writer nw_F(zdd_F);
      nw_F << node(false);

      node_writer nw_T(zdd_T);
      nw_T << node(true);
    }

    it("returns false for Ø on Ø", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      AssertThat(zdd_contains(zdd_F, labels), Is().False());
    });

    it("returns true for { Ø } on Ø", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      AssertThat(zdd_contains(zdd_T, labels), Is().True());
    });

    it("returns false for { Ø } on { 1, 42 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer lw(labels);
        lw << 1 << 42;
      }

      AssertThat(zdd_contains(zdd_T, labels), Is().False());
    });

    const ptr_uint64 terminal_F = ptr_uint64(false);
    const ptr_uint64 terminal_T = ptr_uint64(true);

    shared_levelized_file<zdd::node_t> zdd_x0;
    // { { 0 } }
    /*
            1        ---- x0
           / \
           F T
     */
    { // Garbage collect writers to free write-lock
      node_writer nw(zdd_x0);
      nw << node(0, node::MAX_ID, terminal_F, terminal_T);
    }

    // TODO: tests...

    shared_levelized_file<zdd::node_t> zdd_x1_null;
    // { Ø, { 1 } }
    /*
            1         ---- x1
           / \
           T T
     */
    { // Garbage collect writers to free write-lock
      node_writer nw(zdd_x1_null);
      nw << node(1, node::MAX_ID, terminal_T, terminal_T);
    }

    // TODO: tests...

    shared_levelized_file<zdd::node_t> zdd_1;
    // { Ø, { 0,2 }, { 0,3 } { 1,2 }, { 1,3 }, { 1,2,3 }, { 0,2,3 } }
    /*
                1      ---- x0
               / \
               2  \    ---- x1
              / \ /
              T  3     ---- x2
                / \
               4  5    ---- x3
              / \/ \
              F T  T
     */
    const node n1_5 = node(3, node::MAX_ID,   terminal_T,   terminal_T);
    const node n1_4 = node(3, node::MAX_ID-1, terminal_F,   terminal_T);
    const node n1_3 = node(2, node::MAX_ID,   n1_4.uid(), n1_5.uid());
    const node n1_2 = node(1, node::MAX_ID,   terminal_T,   n1_3.uid());
    const node n1_1 = node(0, node::MAX_ID,   n1_2.uid(), n1_3.uid());

    { // Garbage collect writers to free write-lock
      node_writer nw(zdd_1);
      nw << n1_5 << n1_4 << n1_3 << n1_2 << n1_1;
    }

    it("returns visited root for [1] on Ø", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      AssertThat(zdd_contains(zdd_1, labels), Is().True());
    });

    it("returns visited terminal for [1] on { 0 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().False());
    });

    it("returns visited terminal for [1] on { 1 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 1;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().False());
    });

    it("returns visited terminal for [1] on { 0, 2 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0 << 2;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().True());
    });

    it("returns visited terminal for [1] on { 1, 3 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 1 << 3;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().True());
    });

    it("returns visited terminal for [1] on { 0, 2, 3 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0 << 2 << 3;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().True());
    });

    it("fails on missed label for [1] on { 0, 1, 2 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0 << 1 << 2;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().False());
    });

    it("fails on terminal with unread labels for [1] on { 2 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 2;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().False());
    });

    it("fails on terminal with unread labels for [1] on { 0, 2, 4 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0 << 2 << 4;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().False());
    });

    it("fails on terminal with unread labels for [1] on { 0, 2, 3, 4 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0 << 2 << 3 << 4;
      }

      AssertThat(zdd_contains(zdd_1, labels), Is().False());
    });

    shared_levelized_file<zdd::node_t> zdd_2;
    // { { 6 }, { 2,4 }, { 2,6 }, { 2,4,6 } }
    /*
               1       ---- x2
              / \
              | 2      ---- x4
              |/ \
              3  4     ---- x6
             / \/ \
             F T  T
     */
    const node n2_4 = node(6, node::MAX_ID,   terminal_T,   terminal_T);
    const node n2_3 = node(6, node::MAX_ID-1, terminal_F,   terminal_T);
    const node n2_2 = node(4, node::MAX_ID,   n2_3.uid(), n2_4.uid());
    const node n2_1 = node(2, node::MAX_ID,   n2_3.uid(), n2_2.uid());

    { // Garbage collect writers to free write-lock
      node_writer nw(zdd_2);
      nw << n2_4 << n2_3 << n2_2 << n2_1;
    }

    it("returns visited root for [2] on Ø", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      AssertThat(zdd_contains(zdd_2, labels), Is().False());
    });

    it("returns visited terminal for [2] on { 2 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 2;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().False());
    });

    it("returns visited terminal for [2] on { 6 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 6;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().True());
    });

    it("returns visited terminal for [2] on { 2, 4 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 2 << 4;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().True());
    });

    it("returns visited terminal for [2] on { 2, 4, 6 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 2 << 4 << 6;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().True());
    });

    it("fails on missed label for [2] on { 4, 6 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 4 << 6;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().False());
    });

    it("fails on missed label for [2] on { 2, 3, 4 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 2 << 3 << 4;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().False());
    });

    it("fails on missed label for [2] on { 2, 4, 6, 8 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 2 << 4 << 6 << 8;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().False());
    });

    it("fails on label before root for [2] on { 0, 2, 4 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0 << 2 << 4;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().False());
    });

    it("fails on labels before root for [2] on { 0, 1, 2, 4 }", [&]() {
      adiar::shared_file<zdd::label_t> labels;

      {
        label_writer w(labels);
        w << 0 << 1 << 2 << 4;
      }

      AssertThat(zdd_contains(zdd_2, labels), Is().False());
    });
  });
 });
