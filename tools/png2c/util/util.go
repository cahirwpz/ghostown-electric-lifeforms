package util

import (
	"bytes"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"math"
)

func CutImage(startX, startY, width, height int, img image.Config, pix []uint8) []uint8 {
	offset := img.Width*startY + startX
	out := []uint8{}

	for i := 0; i < height; i++ {
		out = append(out, pix[offset:offset+width]...)
		offset += img.Width
	}

	return out
}

func CleanPalette(pal color.Palette) color.Palette {
	colors := map[color.Color]color.Color{}
	for _, p := range pal {
		if _, ok := colors[p]; !ok {
			colors[p] = p
		}
	}

	out := color.Palette{}
	for _, o := range colors {
		out = append(out, o)
	}
	return out
}

func GetDepth(pix []uint8) int {
	pal := map[uint8]uint8{}
	for _, p := range pix {
		if _, ok := pal[p]; !ok {
			pal[p] = p
		}
	}

	return int(math.Ceil(math.Log2(float64(len(pal)))))
}

func DecodePNG(file []byte) (image.Image, image.Config, error) {
	cfg, err := png.DecodeConfig(bytes.NewReader(file))
	if err != nil {
		return nil, image.Config{}, fmt.Errorf("expected a PNG image, err: %v", err)
	}

	img, err := png.Decode(bytes.NewReader(file))
	if err != nil {
		return nil, image.Config{}, err
	}

	return img, cfg, nil
}

func Planar(pix []uint8, width, height, depth int) []uint16 {
	data := make([]uint16, 0, len(pix))
	padding := make([]uint8, 16-(width&15))

	for offset := 0; offset < width*height; offset = offset + width {
		row := make([]uint8, width+len(padding))
		r := pix[offset : offset+width]
		if width&15 != 0 {
			for i, v := range r {
				row[i] = v
			}
		} else {
			row = r
		}

		for p := 0; p < depth; p++ {
			bits := make([]uint16, len(row))
			for i, byte := range row {
				bits[i] = uint16(byte >> p & 1)
			}
			for i := 0; i < width; i = i + 16 {
				var word uint16 = 0
				for j := 0; j < 16; j++ {
					word = word*2 + bits[i+j]
				}
				data = append(data, word)
			}
		}
	}

	return data
}
