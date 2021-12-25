#ifndef ADIAR_ZDD_ELEM_H
#define ADIAR_ZDD_ELEM_H

#include <optional>

#include <adiar/file.h>
#include <adiar/zdd/zdd.h>

namespace adiar
{
  //////////////////////////////////////////////////////////////////////////////
  /// \brief Retrieves the lexicographically smallest set a in A.
  //////////////////////////////////////////////////////////////////////////////
  std::optional<label_file> zdd_minelem(const zdd &A);

  //////////////////////////////////////////////////////////////////////////////
  /// \brief Retrieves the lexicographically largest set a in A.
  //////////////////////////////////////////////////////////////////////////////
  std::optional<label_file> zdd_maxelem(const zdd &A);
}

#endif // ADIAR_ZDD_ELEM_H
