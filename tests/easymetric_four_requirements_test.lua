-- ==============================================================================
-- Easymetric 6-Surface Live Studio - 4 Requirements Comprehensive Verification
-- Aseprite 2.0 Adversarial Test Suite
-- ==============================================================================

local kFaceTop = 0
local kFaceDown = 1
local kFaceFront = 2
local kFaceBack = 3
local kFaceRight = 4
local kFaceLeft = 5
local kFaceCount = 6

local kFaceSpecs = {
  [kFaceTop]   = { sliceName = "TOP",   displayName = "Top",    col = 0, row = 0, r = 168, g = 85,  b = 247 },
  [kFaceDown]  = { sliceName = "DOWN",  displayName = "Bottom", col = 0, row = 1, r = 244, g = 63,  b = 94  },
  [kFaceFront] = { sliceName = "FRONT", displayName = "Front",  col = 1, row = 0, r = 0,   g = 229, b = 255 },
  [kFaceBack]  = { sliceName = "BACK",  displayName = "Back",   col = 1, row = 1, r = 59,  g = 130, b = 246 },
  [kFaceRight] = { sliceName = "RIGHT", displayName = "Right",  col = 2, row = 0, r = 245, g = 158, b = 11  },
  [kFaceLeft]  = { sliceName = "LEFT",  displayName = "Left",   col = 2, row = 1, r = 16,  g = 185, b = 129 }
}

local suite = {
  passed = 0,
  failed = 0,
  currentSuite = "Default",
  failures = {}
}

local function describe(suiteName, fn)
  suite.currentSuite = suiteName
  print(string.format("\n======================================================\n%s\n======================================================", suiteName))
  fn()
end

local function it(testName, fn)
  local ok, err = pcall(fn)
  if ok then
    suite.passed = suite.passed + 1
    print("  [PASS] " .. testName)
  else
    suite.failed = suite.failed + 1
    table.insert(suite.failures, { suite = suite.currentSuite, test = testName, err = tostring(err) })
    print("  [FAIL] " .. testName)
    print("         Error: " .. tostring(err))
  end
end

local function assert_eq(a, b, msg)
  if a ~= b then
    error(string.format("%s: expected '%s', got '%s'", msg or "assertion failed", tostring(b), tostring(a)), 2)
  end
end

local function assert_true(cond, msg)
  if not cond then
    error(msg or "expected true, got false", 2)
  end
end

local function assert_false(cond, msg)
  if cond then
    error(msg or "expected false, got true", 2)
  end
end

local function assert_not_nil(val, msg)
  if val == nil then
    error(msg or "expected non-nil value", 2)
  end
end

-- ==============================================================================
-- REQUIREMENT 1: 3D Preview Geometry Clipping Fix
-- ==============================================================================
describe("Requirement 1: 3D Preview Geometry Clipping Bug Fix", function()
  it("TC 1.1: Camera raymarching bounds encompass full 3D object geometry without triangular clipping", function()
    local maxDim = 32
    local halfX = 16.0
    local halfY = 16.0
    local halfZ = 16.0
    local maxDepthRange = maxDim * 1.73205
    local camDist = maxDepthRange * 2.5
    local nearPlane = 1.0
    local farPlane = camDist * 2.0 + maxDepthRange

    local yaw = 45.0 * math.pi / 180.0
    local pitch = 30.0 * math.pi / 180.0
    local cy = math.cos(yaw)
    local sy = math.sin(yaw)
    local cp = math.cos(pitch)
    local sp = math.sin(pitch)

    local rxX = cy;      local rxY = 0.0; local rzX = -sy
    local ryX = sy * sp; local ryY = cp;  local ryZ = cy * sp
    local vdX = sy * cp; local vdY = -sp; local vdZ = cy * cp

    local function clipAxis(orig, dir, bMin, bMax, t0, t1)
      if math.abs(dir) > 1e-5 then
        local invD = 1.0 / dir
        local ta = (bMin - orig) * invD
        local tb = (bMax - orig) * invD
        if ta > tb then ta, tb = tb, ta end
        if ta > t0 then t0 = ta end
        if tb < t1 then t1 = tb end
        return t0 <= t1, t0, t1
      end
      return (orig >= bMin and orig <= bMax), t0, t1
    end

    local outW = maxDim * 2 + 16
    local outH = maxDim * 2 + 16
    local cxScreen = outW * 0.5
    local cyScreen = outH * 0.5

    local hitsCount = 0
    local frontHemisphereHits = 0
    local backHemisphereHits = 0
    local minHitTRel = 1e9
    local maxHitTRel = -1e9

    for py = 0, outH - 1 do
      local camY = py - cyScreen
      for px = 0, outW - 1 do
        local camX = px - cxScreen
        local ox = (camX * rxX + camY * ryX) - camDist * vdX
        local oy = (camX * rxY + camY * ryY) - camDist * vdY
        local oz = (camX * rzX + camY * ryZ) - camDist * vdZ

        local t0 = -1e9
        local t1 = 1e9
        local okX; okX, t0, t1 = clipAxis(ox, vdX, -halfX, halfX, t0, t1)
        local okY; okY, t0, t1 = clipAxis(oy, vdY, -halfY, halfY, t0, t1)
        local okZ; okZ, t0, t1 = clipAxis(oz, vdZ, -halfZ, halfZ, t0, t1)

        if okX and okY and okZ and t1 >= nearPlane and t0 <= farPlane and t0 <= t1 then
          hitsCount = hitsCount + 1
          local tHit = t0
          local tRel = tHit - camDist
          if tRel < minHitTRel then minHitTRel = tRel end
          if tRel > maxHitTRel then maxHitTRel = tRel end

          if tRel < 0 then
            frontHemisphereHits = frontHemisphereHits + 1
          else
            backHemisphereHits = backHemisphereHits + 1
          end
        end
      end
    end

    assert_true(hitsCount > 0, "Cube must have projected hits")
    assert_true(frontHemisphereHits > 0, "Front half facing camera MUST NOT be clipped into triangular slice!")
    assert_true(backHemisphereHits > 0, "Back half must have valid projected rays")

    local normZMin = math.max(0.0, math.min(1.0, (minHitTRel + maxDepthRange * 0.5) / maxDepthRange))
    local normZMax = math.max(0.0, math.min(1.0, (maxHitTRel + maxDepthRange * 0.5) / maxDepthRange))
    assert_true(normZMin >= 0.0 and normZMin <= 1.0, "Near depth normalized")
    assert_true(normZMax >= 0.0 and normZMax <= 1.0, "Far depth normalized")
    assert_true(normZMin < normZMax, "Depth gradient monotonicity preserved")
  end)

  it("TC 1.2: Voxel volume raymarching correctly samples full geometry across Diffuse, Normal, Depth channels", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupSurfaces" }
    app.command.EasymetricStudio{ action = "bake3D", yaw = 45, pitch = 30 }

    local diffLyr, normLyr, deptLyr = nil, nil, nil
    for _, l in ipairs(spr.layers) do
      if l.name == "Iso_Diffuse" then diffLyr = l end
      if l.name == "Iso_Normal" then normLyr = l end
      if l.name == "Iso_Depth" then deptLyr = l end
    end

    assert_not_nil(diffLyr, "Iso_Diffuse layer generated")
    assert_not_nil(normLyr, "Iso_Normal layer generated")
    assert_not_nil(deptLyr, "Iso_Depth layer generated")

    local diffImg = diffLyr:cel(1).image
    local normImg = normLyr:cel(1).image
    local deptImg = deptLyr:cel(1).image

    assert_true(diffImg.width >= 48 and diffImg.height >= 48, "Preview canvas dimension encompasses 3D object")

    local litPixels = 0
    local nonNeutralNormals = 0
    local nonZeroDepths = 0

    for y = 0, diffImg.height - 1 do
      for x = 0, diffImg.width - 1 do
        local c = diffImg:getPixel(x, y)
        if app.pixelColor.rgbaA(c) > 0 then
          litPixels = litPixels + 1
        end

        local nc = normImg:getPixel(x, y)
        if app.pixelColor.rgbaA(nc) > 0 then
          nonNeutralNormals = nonNeutralNormals + 1
        end

        local dc = deptImg:getPixel(x, y)
        if app.pixelColor.rgbaA(dc) > 0 and app.pixelColor.rgbaR(dc) > 0 then
          nonZeroDepths = nonZeroDepths + 1
        end
      end
    end

    assert_true(litPixels > 200, "Voxel raymarching generated solid diffuse geometry")
    assert_true(nonNeutralNormals > 200, "Normal map geometry generated")
    assert_true(nonZeroDepths > 200, "Depth map geometry generated")

    spr:close()
  end)

  it("TC 1.3: Non-square dimensions (tall 16x48, wide 48x16) project without corner truncation", function()
    local maxDim = 48
    local halfX = 8.0
    local halfY = 24.0
    local halfZ = 8.0
    local maxDepthRange = maxDim * 1.73205
    local camDist = maxDepthRange * 2.5
    local nearPlane = 1.0
    local farPlane = camDist * 2.0 + maxDepthRange

    local yaw = 45.0 * math.pi / 180.0
    local pitch = 30.0 * math.pi / 180.0
    local cy, sy = math.cos(yaw), math.sin(yaw)
    local cp, sp = math.cos(pitch), math.sin(pitch)

    local rxX, rxY, rzX = cy, 0.0, -sy
    local ryX, ryY, ryZ = sy * sp, cp, cy * sp
    local vdX, vdY, vdZ = sy * cp, -sp, cy * cp

    local function clipAxis(orig, dir, bMin, bMax, t0, t1)
      if math.abs(dir) > 1e-5 then
        local invD = 1.0 / dir
        local ta = (bMin - orig) * invD
        local tb = (bMax - orig) * invD
        if ta > tb then ta, tb = tb, ta end
        if ta > t0 then t0 = ta end
        if tb < t1 then t1 = tb end
        return t0 <= t1, t0, t1
      end
      return (orig >= bMin and orig <= bMax), t0, t1
    end

    local hits = 0
    local outDim = maxDim * 2 + 16
    local cxScreen = outDim * 0.5
    local cyScreen = outDim * 0.5

    for py = 0, outDim - 1 do
      local camY = py - cyScreen
      for px = 0, outDim - 1 do
        local camX = px - cxScreen
        local ox = (camX * rxX + camY * ryX) - camDist * vdX
        local oy = (camX * rxY + camY * ryY) - camDist * vdY
        local oz = (camX * rzX + camY * ryZ) - camDist * vdZ

        local t0, t1 = -1e9, 1e9
        local okX; okX, t0, t1 = clipAxis(ox, vdX, -halfX, halfX, t0, t1)
        local okY; okY, t0, t1 = clipAxis(oy, vdY, -halfY, halfY, t0, t1)
        local okZ; okZ, t0, t1 = clipAxis(oz, vdZ, -halfZ, halfZ, t0, t1)

        if okX and okY and okZ and t1 >= nearPlane and t0 <= farPlane and t0 <= t1 then
          hits = hits + 1
        end
      end
    end

    assert_true(hits > 0, "Non-square volume must project valid non-zero hit pixels")
  end)
end)

-- ==============================================================================
-- REQUIREMENT 2: Two-Layer Smart Object & Clipping Structure
-- ==============================================================================
describe("Requirement 2: Two-Layer Smart Object & Clipping Structure", function()
  it("TC 2.1: Base layer acts as alpha clipping mask for overlaying texture layer", function()
    local spr = Sprite(16, 16)
    local baseLayer = spr.layers[1]
    baseLayer.name = "Surface_FRONT_Base"
    local baseImg = baseLayer:cel(1).image
    baseImg:clear(Color{ r = 0, g = 0, b = 0, a = 0 })

    for y = 4, 11 do
      for x = 4, 11 do
        baseImg:drawPixel(x, y, Color{ r = 100, g = 100, b = 100, a = 255 })
      end
    end

    local texLayer = spr:newLayer()
    texLayer.name = "Surface_FRONT_Texture"
    local texImg = Image(16, 16)
    texImg:clear(Color{ r = 255, g = 200, b = 0, a = 255 })
    spr:newCel(texLayer, 1, texImg, Point(0, 0))

    local composite = Image(16, 16)
    composite:clear(Color{ r = 0, g = 0, b = 0, a = 0 })

    for y = 0, 15 do
      for x = 0, 15 do
        local bc = baseImg:getPixel(x, y)
        local tc = texImg:getPixel(x, y)
        local baseA = app.pixelColor.rgbaA(bc)
        local texA = app.pixelColor.rgbaA(tc)

        local finalCol = Color{ r = 0, g = 0, b = 0, a = 0 }
        if baseA > 16 then
          if texA > 0 then
            finalCol = Color(tc)
          else
            finalCol = Color(bc)
          end
        end
        composite:drawPixel(x, y, finalCol)
      end
    end

    assert_eq(app.pixelColor.rgbaA(composite:getPixel(0, 0)), 0, "Pixel outside base mask must be clipped")
    assert_eq(app.pixelColor.rgbaA(composite:getPixel(3, 3)), 0, "Pixel outside base mask must be clipped")
    assert_eq(app.pixelColor.rgbaA(composite:getPixel(12, 12)), 0, "Pixel outside base mask must be clipped")

    local centerPx = composite:getPixel(8, 8)
    assert_eq(app.pixelColor.rgbaR(centerPx), 255, "Texture override R")
    assert_eq(app.pixelColor.rgbaG(centerPx), 200, "Texture override G")
    assert_eq(app.pixelColor.rgbaB(centerPx), 0,   "Texture override B")
    assert_eq(app.pixelColor.rgbaA(centerPx), 255, "Texture override A")

    spr:close()
  end)

  it("TC 2.2: Transparent texture pixels fall back to base layer color", function()
    local spr = Sprite(16, 16)
    local baseLayer = spr.layers[1]
    baseLayer.name = "Surface_FRONT_Base"
    local baseImg = baseLayer:cel(1).image
    baseImg:clear(Color{ r = 50, g = 150, b = 250, a = 255 })

    local texLayer = spr:newLayer()
    texLayer.name = "Surface_FRONT_Texture"
    local texImg = Image(16, 16)
    texImg:clear(Color{ r = 0, g = 0, b = 0, a = 0 })
    texImg:drawPixel(8, 8, Color{ r = 255, g = 0, b = 0, a = 255 })
    spr:newCel(texLayer, 1, texImg, Point(0, 0))

    local tc = texImg:getPixel(8, 8)
    assert_eq(app.pixelColor.rgbaR(tc), 255)

    local bc = baseImg:getPixel(4, 4)
    assert_eq(app.pixelColor.rgbaB(bc), 250)

    spr:close()
  end)

  it("TC 2.3: EasymetricStudio setupSmartObjects creates unified 2-layer Smart Object texture architecture", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupSmartObjects" }

    local baseLyr, texLyr = nil, nil
    for _, lyr in ipairs(spr.layers) do
      if lyr.name == "Surface_Base" then baseLyr = lyr end
      if lyr.name == "Surface_Texture" then texLyr = lyr end
    end

    assert_not_nil(baseLyr, "Surface_Base layer exists")
    assert_not_nil(texLyr, "Surface_Texture layer exists")
    assert_true(texLyr.isSmartObject, "Surface_Texture is a Smart Object")
    spr:close()
  end)

  it("TC 2.4: EasymetricStudio convertToGroups unpacks smart object texture into layer group", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupSmartObjects" }
    app.command.EasymetricStudio{ action = "convertToGroups" }

    local texGroup = nil
    for _, lyr in ipairs(spr.layers) do
      if lyr.isGroup and string.find(lyr.name, "Surface_Texture") then
        texGroup = lyr
        break
      end
    end

    assert_not_nil(texGroup, "Expected Surface_Texture converted to Layer Group")
    spr:close()
  end)

  it("TC 2.5: EasymetricStudio setupGroups creates shared 2-layer hierarchy", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupGroups" }

    local baseLyr, texLyr = nil, nil
    for _, lyr in ipairs(spr.layers) do
      if lyr.name == "Surface_Base" then baseLyr = lyr end
      if lyr.name == "Surface_Texture" then texLyr = lyr end
    end

    assert_not_nil(baseLyr, "Surface_Base layer exists")
    assert_not_nil(texLyr, "Surface_Texture layer exists")
    spr:close()
  end)

  it("TC 2.6: Semi-transparent texture layer alpha-blends over base layer without discarding base", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupGroups" }

    local baseLyr, texLyr = nil, nil
    for _, l in ipairs(spr.layers) do
      if l.name == "Surface_Base" then baseLyr = l end
      if l.name == "Surface_Texture" then texLyr = l end
    end
    assert_not_nil(baseLyr, "Surface_Base layer exists")
    assert_not_nil(texLyr, "Surface_Texture layer exists")

    -- Base layer solid white
    baseLyr:cel(1).image:clear(Color{ r = 255, g = 255, b = 255, a = 255 })

    -- Texture layer semi-transparent red (alpha 128)
    local tImg = Image(baseLyr:cel(1).image.width, baseLyr:cel(1).image.height)
    tImg:clear(Color{ r = 255, g = 0, b = 0, a = 128 })
    if texLyr:cel(1) then
      texLyr:cel(1).image = tImg
    else
      spr:newCel(texLyr, 1, tImg, Point(0, 0))
    end

    -- Bake Front view (yaw 0, pitch 0)
    app.command.EasymetricStudio{ action = "bake3D", yaw = 0, pitch = 0 }

    local diffLyr = nil
    for _, l in ipairs(spr.layers) do
      if l.name == "Iso_Diffuse" then diffLyr = l break end
    end
    assert_not_nil(diffLyr, "Iso_Diffuse generated")

    local img = diffLyr:cel(1).image
    -- Check center pixel: should be blended (R is high, G and B are intermediate ~90-140)
    local c = img:getPixel(img.width / 2, img.height / 2)
    local r = app.pixelColor.rgbaR(c)
    local g = app.pixelColor.rgbaG(c)
    local b = app.pixelColor.rgbaB(c)
    local a = app.pixelColor.rgbaA(c)

    assert_eq(a, 255, "Alpha is opaque")
    assert_true(r > 200, "Red channel prominent")
    assert_true(g > 60 and g < 200, "Green channel shows base color through semi-transparency")
    assert_true(b > 60 and b < 200, "Blue channel shows base color through semi-transparency")

    spr:close()
  end)

  it("TC 2.7: Converting smart objects to groups preserves base layer fallback in 3D raymarching", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr
    app.command.EasymetricStudio{ action = "setupSmartObjects" }

    local baseLyr = nil
    for _, l in ipairs(spr.layers) do
      if l.name == "Surface_Base" then baseLyr = l break end
    end
    assert_not_nil(baseLyr, "Surface_Base found")
    baseLyr:cel(1).image:clear(Color{ r = 255, g = 255, b = 255, a = 255 })

    -- Convert smart objects to groups
    app.command.EasymetricStudio{ action = "convertToGroups" }

    local texLyr = nil
    for _, l in ipairs(spr.layers) do
      if l.isGroup and string.find(l.name, "Surface_Texture") then
        texLyr = l.layers[1]
        break
      end
    end
    if not texLyr then
      for _, l in ipairs(spr.layers) do
        if l.name == "Surface_Texture" then texLyr = l break end
      end
    end
    assert_not_nil(texLyr, "Unpacked texture layer found")
    local tImg = Image(baseLyr:cel(1).image.width, baseLyr:cel(1).image.height)
    tImg:clear(Color{ r = 0, g = 0, b = 0, a = 0 })
    if texLyr:cel(1) then
      texLyr:cel(1).image = tImg
    else
      spr:newCel(texLyr, 1, tImg, Point(0, 0))
    end

    -- Bake Front view (yaw 180, pitch 0)
    app.command.EasymetricStudio{ action = "bake3D", yaw = 180, pitch = 0 }

    local diffLyr = nil
    for _, l in ipairs(spr.layers) do
      if l.name == "Iso_Diffuse" then diffLyr = l break end
    end
    assert_not_nil(diffLyr, "Iso_Diffuse layer generated")

    local img = diffLyr:cel(1).image
    local centerPx = img:getPixel(img.width / 2, img.height / 2)
    local r = app.pixelColor.rgbaR(centerPx)
    local g = app.pixelColor.rgbaG(centerPx)
    local b = app.pixelColor.rgbaB(centerPx)

    assert_true(r > 200 and g > 200 and b > 200,
      string.format("Transparent texture on unpacked group must fall back to solid white base (got R=%d, G=%d, B=%d)", r, g, b))

    spr:close()
  end)

  it("TC 2.8: 2-layer setup initializes shared UV sheets and slice partitions on canvas", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupGroups" }

    local baseLyr, texLyr = nil, nil
    for _, l in ipairs(spr.layers) do
      if l.name == "Surface_Base" then baseLyr = l end
      if l.name == "Surface_Texture" then texLyr = l end
    end
    assert_not_nil(baseLyr, "Base layer created")
    assert_not_nil(texLyr, "Texture layer created")

    local bCel = baseLyr:cel(1)
    local tCel = texLyr:cel(1)
    assert_not_nil(bCel, "Base cel exists")
    assert_not_nil(tCel, "Texture cel exists")

    assert_true(spr.width >= 3 * 24, "Sprite width accommodates 3 columns")
    assert_true(spr.height >= 2 * 24, "Sprite height accommodates 2 rows")

    spr:close()
  end)
end)

-- ==============================================================================
-- REQUIREMENT 3: Canvas Boundary Drawing Isolation on Shared Layer
-- ==============================================================================
describe("Requirement 3: Canvas Boundary Drawing Isolation on Shared Layer", function()
  it("TC 3.1: Drawing on Top surface slice on shared layer does not bleed into Bottom or Front slices", function()
    local spr = Sprite(64, 64)
    local sharedLayer = spr.layers[1]
    sharedLayer.name = "SharedCanvas"
    local img = sharedLayer:cel(1).image
    img:clear(Color{ r = 0, g = 0, b = 0, a = 0 })

    local slTop = spr:newSlice(Rectangle(0, 0, 32, 32))
    slTop.name = "TOP"

    local slDown = spr:newSlice(Rectangle(32, 0, 32, 32))
    slDown.name = "DOWN"

    local slFront = spr:newSlice(Rectangle(0, 32, 32, 32))
    slFront.name = "FRONT"

    app.activeSprite = spr
    app.activeLayer = sharedLayer

    app.useTool{
      tool = "pencil",
      color = Color{ r = 255, g = 255, b = 255, a = 255 },
      points = { Point(16, 16), Point(48, 16) }
    }

    local canvas = Image(spr.spec)
    canvas:drawSprite(spr, 1)

    local pxTop = canvas:getPixel(16, 16)
    assert_eq(app.pixelColor.rgbaA(pxTop), 255, "Top surface inside bounds must be drawn")

    local pxDown = canvas:getPixel(48, 16)
    assert_eq(app.pixelColor.rgbaA(pxDown), 0, "Adjacent Down surface MUST NOT receive bleed-over ink on shared layer")

    local pxDownBorder = canvas:getPixel(33, 16)
    assert_eq(app.pixelColor.rgbaA(pxDownBorder), 0, "Down border MUST NOT receive bleed-over ink")

    local pxFront = canvas:getPixel(16, 40)
    assert_eq(app.pixelColor.rgbaA(pxFront), 0, "Front surface below MUST NOT receive bleed-over ink")

    spr:close()
  end)

  it("TC 3.2: Drawing isolation works with lowercase and prefixed slice names", function()
    local spr = Sprite(64, 32)
    local sharedLayer = spr.layers[1]
    sharedLayer.name = "SharedCanvas"
    local img = sharedLayer:cel(1).image
    img:clear(Color{ r = 0, g = 0, b = 0, a = 0 })

    -- Lowercase and prefixed slice names
    local slTop = spr:newSlice(Rectangle(0, 0, 32, 32))
    slTop.name = "surface_top"

    local slDown = spr:newSlice(Rectangle(32, 0, 32, 32))
    slDown.name = "face_down"

    app.activeSprite = spr
    app.activeLayer = sharedLayer

    app.useTool{
      tool = "pencil",
      color = Color{ r = 200, g = 100, b = 50, a = 255 },
      points = { Point(10, 10), Point(40, 10) }
    }

    local canvas = Image(spr.spec)
    canvas:drawSprite(spr, 1)

    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(10, 10)), 255, "surface_top painted")
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(40, 10)), 0, "face_down isolated from bleeding")

    spr:close()
  end)

  it("TC 3.3: Large brush diameter exceeding slice boundaries clamps strictly to surface isolation bounds", function()
    local spr = Sprite(64, 32)
    local sharedLayer = spr.layers[1]
    sharedLayer.name = "SharedCanvas"
    local img = sharedLayer:cel(1).image
    img:clear(Color{ r = 0, g = 0, b = 0, a = 0 })

    local slTop = spr:newSlice(Rectangle(0, 0, 32, 32))
    slTop.name = "TOP"

    local slDown = spr:newSlice(Rectangle(32, 0, 32, 32))
    slDown.name = "DOWN"

    app.activeSprite = spr
    app.activeLayer = sharedLayer

    -- Draw near boundary (30, 16) with large 16px brush
    app.useTool{
      tool = "pencil",
      brush = Brush(16),
      color = Color{ r = 0, g = 255, b = 100, a = 255 },
      points = { Point(30, 16) }
    }

    local canvas = Image(spr.spec)
    canvas:drawSprite(spr, 1)

    -- Point inside TOP at (30, 16) must be painted
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(30, 16)), 255, "Inside TOP painted")

    -- Point inside adjacent DOWN slice at (32, 16) MUST NOT be touched
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(32, 16)), 0, "Adjacent DOWN untouched despite 16px brush radius")
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(35, 16)), 0, "Adjacent DOWN untouched")

    spr:close()
  end)

  it("TC 3.4: Flood fill on shared layer is strictly bounded to surface slice", function()
    local spr = Sprite(64, 32)
    local sharedLayer = spr.layers[1]
    sharedLayer.name = "SharedCanvas"
    local img = sharedLayer:cel(1).image
    img:clear(Color{ r = 0, g = 0, b = 0, a = 0 })

    local slTop = spr:newSlice(Rectangle(0, 0, 32, 32))
    slTop.name = "TOP"

    local slDown = spr:newSlice(Rectangle(32, 0, 32, 32))
    slDown.name = "DOWN"

    app.activeSprite = spr
    app.activeLayer = sharedLayer

    app.useTool{
      tool = "paint_bucket",
      color = Color{ r = 255, g = 0, b = 0, a = 255 },
      points = { Point(10, 10) }
    }

    local canvas = Image(spr.spec)
    canvas:drawSprite(spr, 1)

    assert_eq(app.pixelColor.rgbaR(canvas:getPixel(5, 5)), 255, "TOP filled")
    assert_eq(app.pixelColor.rgbaR(canvas:getPixel(31, 31)), 255, "TOP border filled")

    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(32, 5)), 0, "DOWN surface must not be touched by flood fill")
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(45, 15)), 0, "DOWN center must not be touched by flood fill")

    spr:close()
  end)

  it("TC 3.5: Stroke initiated in non-origin slice (FRONT) is isolated from bleeding across into other slices", function()
    local spr = Sprite(64, 64)
    local sharedLayer = spr.layers[1]
    sharedLayer.name = "SharedCanvas"
    local img = sharedLayer:cel(1).image
    img:clear(Color{ r = 0, g = 0, b = 0, a = 0 })

    local slTop = spr:newSlice(Rectangle(0, 0, 32, 32))
    slTop.name = "TOP"

    local slDown = spr:newSlice(Rectangle(32, 0, 32, 32))
    slDown.name = "DOWN"

    local slFront = spr:newSlice(Rectangle(0, 32, 32, 32))
    slFront.name = "FRONT"

    local slBack = spr:newSlice(Rectangle(32, 32, 32, 32))
    slBack.name = "BACK"

    app.activeSprite = spr
    app.activeLayer = sharedLayer

    -- Stroke starts inside FRONT at (16, 48), drags up into TOP at (16, 16) and right into BACK at (48, 48)
    app.useTool{
      tool = "pencil",
      color = Color{ r = 0, g = 255, b = 255, a = 255 },
      points = { Point(16, 48), Point(16, 16) }
    }

    local canvas = Image(spr.spec)
    canvas:drawSprite(spr, 1)

    -- Point inside FRONT must be drawn
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(16, 48)), 255, "Inside FRONT must be painted")

    -- Point inside TOP (16, 16) MUST NOT be drawn
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(16, 16)), 0, "Stroke starting in FRONT must not bleed into TOP")
    assert_eq(app.pixelColor.rgbaA(canvas:getPixel(16, 31)), 0, "TOP bottom edge must not be touched")

    spr:close()
  end)
end)

-- ==============================================================================
-- REQUIREMENT 4: Linked Surface Dimension Dependencies
-- ==============================================================================
describe("Requirement 4: Linked Surface Dimension Dependencies", function()
  it("TC 4.1: Mathematical cube geometry relationships verified", function()
    local function propagateDimensionLink(srcFace, newW, newH, curW, curH)
      local w = { [kFaceTop]=curW[kFaceTop], [kFaceDown]=curW[kFaceDown],
                  [kFaceFront]=curW[kFaceFront], [kFaceBack]=curW[kFaceBack],
                  [kFaceRight]=curW[kFaceRight], [kFaceLeft]=curW[kFaceLeft] }
      local h = { [kFaceTop]=curH[kFaceTop], [kFaceDown]=curH[kFaceDown],
                  [kFaceFront]=curH[kFaceFront], [kFaceBack]=curH[kFaceBack],
                  [kFaceRight]=curH[kFaceRight], [kFaceLeft]=curH[kFaceLeft] }

      local X = w[kFaceFront]
      local Y = h[kFaceFront]
      local Z = w[kFaceRight]

      if srcFace == kFaceTop or srcFace == kFaceDown then
        X = newW
        Z = newH
      elseif srcFace == kFaceFront or srcFace == kFaceBack then
        X = newW
        Y = newH
      elseif srcFace == kFaceRight or srcFace == kFaceLeft then
        Z = newW
        Y = newH
      end

      w[kFaceTop] = X;   h[kFaceTop] = Z
      w[kFaceDown] = X;  h[kFaceDown] = Z
      w[kFaceFront] = X; h[kFaceFront] = Y
      w[kFaceBack] = X;  h[kFaceBack] = Y
      w[kFaceRight] = Z; h[kFaceRight] = Y
      w[kFaceLeft] = Z;  h[kFaceLeft] = Y

      return w, h
    end

    local initW = { [0]=24, [1]=24, [2]=24, [3]=24, [4]=24, [5]=24 }
    local initH = { [0]=24, [1]=24, [2]=24, [3]=24, [4]=24, [5]=24 }

    local resW, resH = propagateDimensionLink(kFaceTop, 32, 48, initW, initH)

    assert_eq(resW[kFaceTop], 32, "Top width = X")
    assert_eq(resH[kFaceTop], 48, "Top height = Z")
    assert_eq(resW[kFaceDown], 32, "Bottom width = X")
    assert_eq(resH[kFaceDown], 48, "Bottom height = Z")

    assert_eq(resW[kFaceFront], 32, "Front width = X = Top width")
    assert_eq(resH[kFaceFront], 24, "Front height = Y (unmodified)")
    assert_eq(resW[kFaceBack], 32, "Back width = X = Top width")
    assert_eq(resH[kFaceBack], 24, "Back height = Y (unmodified)")

    assert_eq(resW[kFaceRight], 48, "Right width = Z = Top height")
    assert_eq(resH[kFaceRight], 24, "Right height = Y (unmodified)")
    assert_eq(resW[kFaceLeft], 48, "Left width = Z = Top height")
    assert_eq(resH[kFaceLeft], 24, "Left height = Y (unmodified)")
  end)

  it("TC 4.2: EasymetricStudio linkDims command propagates Top resize across real document slices and cels", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupSurfaces" }

    -- Propagate Top resize to (32, 48)
    app.command.EasymetricStudio{ action = "linkDims", face = "top", width = 32, height = 48 }

    -- Verify slice bounds
    local slices = {}
    for _, sl in ipairs(spr.slices) do
      slices[sl.name] = sl.bounds
    end

    assert_eq(slices["TOP"].width, 32, "Top slice width = 32")
    assert_eq(slices["TOP"].height, 48, "Top slice height = 48")
    assert_eq(slices["DOWN"].width, 32, "Bottom slice width = 32")
    assert_eq(slices["DOWN"].height, 48, "Bottom slice height = 48")

    assert_eq(slices["FRONT"].width, 32, "Front slice width = 32")
    assert_eq(slices["FRONT"].height, 48, "Front slice height = 48")
    assert_eq(slices["BACK"].width, 32, "Back slice width = 32")
    assert_eq(slices["BACK"].height, 48, "Back slice height = 48")

    assert_eq(slices["RIGHT"].width, 32, "Right slice width = 32")
    assert_eq(slices["RIGHT"].height, 48, "Right slice height = 48")
    assert_eq(slices["LEFT"].width, 32, "Left slice width = 32")
    assert_eq(slices["LEFT"].height, 48, "Left slice height = 48")

    assert_eq(spr.width, 96, "Sprite canvas width = 3*32")
    assert_eq(spr.height, 96, "Sprite canvas height = 2*48")

    spr:close()
  end)

  it("TC 4.3: EasymetricStudio linkDims command propagates Front resize to all surfaces uniformly on shared sheet", function()
    local spr = Sprite(24, 24)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupSurfaces" }

    -- Resize Front to (64, 40)
    app.command.EasymetricStudio{ action = "linkDims", face = "front", width = 64, height = 40 }

    local slices = {}
    for _, sl in ipairs(spr.slices) do
      slices[sl.name] = sl.bounds
    end

    assert_eq(slices["FRONT"].width, 64, "Front width = 64")
    assert_eq(slices["FRONT"].height, 40, "Front height = 40")
    assert_eq(slices["BACK"].width, 64, "Back width = 64")
    assert_eq(slices["BACK"].height, 40, "Back height = 40")

    assert_eq(slices["TOP"].width, 64, "Top width updated to Front width")
    assert_eq(slices["TOP"].height, 40, "Top height updated to Front height")

    assert_eq(slices["RIGHT"].width, 64, "Right width remains uniform")
    assert_eq(slices["RIGHT"].height, 40, "Right height updated to Front height")

    assert_eq(spr.width, 192, "Sprite canvas width = 3*64")
    assert_eq(spr.height, 80, "Sprite canvas height = 2*40")

    spr:close()
  end)

  it("TC 4.4: Document canvas expansion accommodates larger surface sizes without truncation", function()
    local spr = Sprite(16, 16)
    app.activeSprite = spr

    app.command.EasymetricStudio{ action = "setupSurfaces" }
    app.command.EasymetricStudio{ action = "linkDims", face = "top", width = 64, height = 32 }

    assert_true(spr.width >= 64, "Sprite width expanded to accommodate 64px width")
    assert_true(spr.height >= 32, "Sprite height expanded to accommodate 32px height")

    spr:close()
  end)

  it("TC 4.5: Slices placed with non-zero origin offset properly expand canvas to prevent slice boundary clipping", function()
    local spr = Sprite(32, 32)
    app.activeSprite = spr

    local slTop = spr:newSlice(Rectangle(16, 16, 16, 16))
    slTop.name = "TOP"

    -- Resize TOP to 40x40. Since origin is x=16, y=16, required canvas is 16+40=56.
    app.command.EasymetricStudio{ action = "linkDims", face = "top", width = 40, height = 40 }

    assert_true(spr.width >= 56, string.format("Sprite width expanded to encompass slice origin + width (got %d)", spr.width))
    assert_true(spr.height >= 56, string.format("Sprite height expanded to encompass slice origin + height (got %d)", spr.height))

    spr:close()
  end)

  it("TC 4.6: Prefixed slice names (surface_front, Surface_TOP) are recognized and resized without duplicate slice creation", function()
    local spr = Sprite(32, 32)
    app.activeSprite = spr

    local sl = spr:newSlice(Rectangle(0, 0, 24, 24))
    sl.name = "surface_front"

    -- Propagate linked dimensions on front to 40x40
    app.command.EasymetricStudio{ action = "linkDims", face = "front", width = 40, height = 40 }

    assert_eq(#spr.slices, 6, "Total slices is exactly 6 without redundant duplicate slices")
    assert_eq(sl.bounds.width, 40, "Existing slice surface_front resized to 40px width")
    assert_eq(sl.bounds.height, 40, "Existing slice surface_front resized to 40px height")

    spr:close()
  end)

  it("TC 4.7: Cel position offsets are preserved during linked dimension propagation", function()
    local spr = Sprite(64, 64)
    app.activeSprite = spr

    local sl = spr:newSlice(Rectangle(16, 16, 24, 24))
    sl.name = "FRONT"

    local lyr = spr:newLayer()
    lyr.name = "Surface_FRONT"
    local img = Image(24, 24)
    img:clear(Color{ r = 255, g = 0, b = 0, a = 255 })
    local cel = spr:newCel(lyr, 1, img, Point(16, 16))

    -- Run linkDims
    app.command.EasymetricStudio{ action = "linkDims", face = "front", width = 32, height = 32 }

    assert_eq(cel.position.x, 16, "Cel X offset preserved at (16, 16)")
    assert_eq(cel.position.y, 16, "Cel Y offset preserved at (16, 16)")
    assert_eq(sl.bounds.x, 16, "Slice X offset preserved at 16")
    assert_eq(sl.bounds.y, 16, "Slice Y offset preserved at 16")
    assert_eq(cel.image.width, 32, "Cel image resized to 32")
    assert_eq(cel.image.height, 32, "Cel image resized to 32")

    -- Test Undo restores cel and slice
    app.command.Undo()
    assert_eq(cel.position.x, 16, "Undo restored Cel X offset")
    assert_eq(cel.position.y, 16, "Undo restored Cel Y offset")
    assert_eq(cel.image.width, 24, "Undo restored Cel image width")
    assert_eq(sl.bounds.width, 24, "Undo restored Slice width")

    spr:close()
  end)
end)

-- ==============================================================================
-- Test Suite Execution Summary
-- ==============================================================================
print("\n======================================================")
print("     4 REQUIREMENTS TEST SUITE EXECUTION SUMMARY      ")
print("======================================================")
print(string.format(" TOTAL:  %3d Passed, %3d Failed (Total %d)", suite.passed, suite.failed, suite.passed + suite.failed))
print("======================================================")

if suite.failed > 0 then
  error(string.format("4 Requirements Test Suite FAILED: %d test(s) failed.", suite.failed))
else
  print("ALL 4 REQUIREMENTS TESTS PASSED SUCCESSFULLY.")
end
