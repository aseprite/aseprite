-- ASE -- Allegro Sprite Editor
-- Copyright (C) 2001-2005, 2007 by David A. Capello

----------------------------------------------------------------------
-- Convolution Matrix

-- you should use set_tiled_mode() before
function ConvolutionMatrix(name, r, g, b, k, a, index)
  local image, x, y = GetImage2 (current_sprite)
  if image then
    local effect = effect_new(current_sprite, "convolution_matrix")
    if effect then
      local convmatr = get_convmatr_by_name(name)
      if convmatr then
        set_convmatr(convmatr)
	effect_set_target(effect, r, g, b, k, a, index)
	effect_apply_to_image(effect, image, x, y)
      end
      effect_free(effect)
    end
  end
end

function ConvolutionMatrixRGB(name)
  ConvolutionMatrix(name, true, true, true, false, false, false)
end

function ConvolutionMatrixRGBA(name)
  ConvolutionMatrix(name, true, true, true, false, true, false)
end

function ConvolutionMatrixGray(name)
  ConvolutionMatrix(name, false, false, false, true, false, false)
end

function ConvolutionMatrixGrayA(name)
  ConvolutionMatrix(name, false, false, false, true, true, false)
end

function ConvolutionMatrixIndex(name)
  ConvolutionMatrix(name, false, false, false, false, false, true)
end

function ConvolutionMatrixAlpha(name)
  ConvolutionMatrix(name, false, false, false, false, true, false)
end

----------------------------------------------------------------------
-- Color Curve

function _ColorCurve(curve, r, g, b, k, a, index)
  local image, x, y = GetImage2 (current_sprite)
  if image then
    local effect = effect_new(current_sprite, "color_curve")
    if effect then
      set_color_curve(curve)
      effect_set_target(effect, r, g, b, k, a, index)
      effect_apply_to_image(effect, image, x, y)
      effect_free(effect)
    end
  end
end

function _ColorCurveRGB(curve)
  _ColorCurve(curve, true, true, true, false, false, false)
end

function _ColorCurveRGBA(curve)
  _ColorCurve(curve, true, true, true, false, true, false)
end

function _ColorCurveGray(curve)
  _ColorCurve(curve, false, false, false, true, false, false)
end

function _ColorCurveGrayA(curve)
  _ColorCurve(curve, false, false, false, true, true, false)
end

function _ColorCurveIndex(curve)
  _ColorCurve(curve, false, false, false, false, false, true)
end

function _ColorCurveAlpha(curve)
  _ColorCurve(curve, false, false, false, false, true, false)
end

----------------------------------------------------------------------
-- Color Curve(with array)

function ColorCurve(array, r, g, b, k, a, index)
  local curve = curve_new(CURVE_LINEAR)
  local c

  for c in array do
    curve_add_point(curve, curve_point_new(array[c], array[c+1]))
    c = c+1
  end

  _ColorCurve(curve, r, g, b, k, a, index)
  curve_free(curve)
end

function ColorCurveRGB(array)
  ColorCurve(array, true, true, true, false, false, false)
end

function ColorCurveRGBA(array)
  ColorCurve(array, true, true, true, false, true, false)
end

function ColorCurveGray(array)
  ColorCurve(array, false, false, false, true, false, false)
end

function ColorCurveGrayA(array)
  ColorCurve(array, false, false, false, true, true, false)
end

function ColorCurveIndex(array)
  ColorCurve(array, false, false, false, false, false, true)
end

function ColorCurveAlpha(array)
  ColorCurve(array, false, false, false, false, true, false)
end
