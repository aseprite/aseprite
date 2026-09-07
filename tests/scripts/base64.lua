-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  assert(app.base64.encode("") == "")
  assert(app.base64.decode("") == "")
  assert(app.base64.encode("hello_test!") == "aGVsbG9fdGVzdCE=")
  assert(app.base64.decode("aGVsbG9fdGVzdCE=") == "hello_test!")
end
