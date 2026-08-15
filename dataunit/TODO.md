- Default values on fields nested inside a subunit do not survive a serialize/deserialize
  round-trip (e.g. DB write then read back), even though they display correctly in memory right
  after construction. Root cause: `SubunitT::deserialize()` (`include/hatn/dataunit/fields/subunit.h`,
  around line 281) calls `fieldClear()` before parsing incoming wire bytes, but `fieldClear()` (the
  base `FieldTmplBytes::fieldClear()` in `include/hatn/dataunit/fields/bytes.h`) just empties the
  field's byte buffer — it does not reapply the field's default the way `fieldReset()` does (only
  `FieldDefault<...>::reset()`/`fieldReset()`, in `include/hatn/dataunit/fields/fieldtraits.h`,
  re-runs `fillDefault()`). A field that was never explicitly `.set()` is therefore absent from the
  serialized wire bytes (only `isSet()` fields get written) and, on deserialize, gets wiped by
  `fieldClear()` with nothing in the wire bytes to restore it — so it reads back as empty/default-
  typed-zero instead of the declared default, on every deserialize, not just a second/reused one.
  Affects both the string-default specialization and the analogous scalar-default specialization
  (`fieldtraits.h`, the non-string default path around lines 197-253) the same way.

  Consider either: (a) making `SubunitT::deserialize()` call `fieldReset()` instead of
  `fieldClear()` before parsing, or (b) making `FieldDefault<...>::clear()`/`fieldClear()` itself
  reapply `fillDefault()` so `clear()` and `reset()` converge for default-valued fields. Whichever
  fix is chosen must not break the wire-format assumption that only `isSet()` fields get serialized.

  Found via a whitemclient bug hunt: `files2::file_key_info::kdf_info` (declared with default
  `DefaultKdfInfo`) came back as an empty string after every DB round-trip because
  `FileCrypto::fillKeyInfo()` relied on the default instead of setting it explicitly. Worked around
  locally in `whitemclient/files2/filecrypto.cpp` by explicitly setting `kdf_info`; the same pattern
  likely affects `key_bunch::kdf_algorithm` (`files2models.h`) and any other subunit-nested default-
  valued field across the codebase that isn't explicitly set before its first serialize.
