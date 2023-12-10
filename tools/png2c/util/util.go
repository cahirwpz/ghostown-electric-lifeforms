package util

import (
	"bytes"
	"fmt"
	"image"
	"image/color"
	"image/png"
	"log"
	"math"
	"slices"
	"strings"
	"text/template"
)

func CompileTemplate(tpl string, data any) string {
	tmpl, err := template.New("template").Parse(tpl)
	if err != nil {
		log.Panic(err)
	}

	var buf strings.Builder
	err = tmpl.Execute(&buf, data)
	if err != nil {
		log.Panic(err)
	}

	return buf.String()
}

func CutImage(startX, startY, width, height int, img image.Config, pix []uint8) []uint8 {
	offset := img.Width*startY + startX
	out := []uint8{}

	for i := 0; i < height; i++ {
		out = append(out, pix[offset:offset+width]...)
		offset += img.Width
	}

	return out
}

func CleanPalette(pix []uint8, pal color.Palette) color.Palette {
	ci := slices.Max(pix) + 1

	return pal[0:ci]
}

func GetDepth(pix []uint8) int {
	return int(math.Ceil(math.Log2(float64(slices.Max(pix) + 1))))
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

func RGB12(c color.Color) uint {
	r, g, b, _ := c.RGBA() // 16-bit components
	return uint(((r & 0xf000) >> 4) | ((g & 0xf000) >> 8) | ((b & 0xf000) >> 12))
}
