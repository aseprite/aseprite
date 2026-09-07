-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  assert(app.base64.encode("") == "")
  assert(app.base64.decode("") == "")
  assert(app.base64.encode("hello_test!") == "aGVsbG9fdGVzdCE=")
  assert(app.base64.decode("aGVsbG9fdGVzdCE=") == "hello_test!")

  local url = app.url
  if url then
    assert(url.encode("h&llo=wr?d") == "h%26llo%3Dwr%3Fd")
    assert(url.decode("h%26llo%3Dwr%3Fd") == "h&llo=wr?d")
    assert(url.encode("テスト") == "%E3%83%86%E3%82%B9%E3%83%88")
    assert(url.encode("") == "")
    assert(url.decode("") == "")
  end

  local net = app.net
  if net then
    -- Invalid inputs
    assert(not pcall(function()
      net.fetch{} -- Nothing
    end))
    assert(not pcall(function()
      net.fetch{ onreceive=function() end } -- No URL
    end))
    assert(not pcall(function()
      net.fetch{ onreceive=function() end, url="/invalid_url" } -- Invalid URL
    end))
    assert(not pcall(function()
      net.fetch{ url="http://localhost/" } -- No callback
    end))
  end
end
