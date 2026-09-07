-- Copyright (C) 2026-present  Igara Studio S.A.
--
-- This file is released under the terms of the MIT license.
-- Read LICENSE.txt for more information.

do
  if (net ~= nil) then
    assert(net.toBase64("hello_test!") == "aGVsbG9fdGVzdCE=")
    assert(net.fromBase64("aGVsbG9fdGVzdCE=") == "hello_test!")
    assert(net.urlEncode("h&llo=wr?d") == "h%26llo%3Dwr%3Fd")
    assert(net.urlDecode("h%26llo%3Dwr%3Fd") == "h&llo=wr?d")
    assert(net.urlEncode("テスト") == "%E3%83%86%E3%82%B9%E3%83%88")
    assert(net.toBase64("") == "")
    assert(net.fromBase64("") == "")
    assert(net.urlEncode("") == "")
    assert(net.urlDecode("") == "")

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
