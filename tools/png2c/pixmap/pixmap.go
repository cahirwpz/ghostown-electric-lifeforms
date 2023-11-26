package pixmap

import (
	_ "embed"
	"fmt"
	"image"
	"image/color"
	"strings"
	"text/template"

	"ghostown.pl/png2c/util"
)

//go:embed template.tpl
var tpl string

func Make(in image.Image, cfg image.Config, opts map[string]any) string {
	o := bindParams(opts)
	out, err := template.New("bitmap_template").Parse(tpl)
	if err != nil {
		panic(err)
	}

	if o.Bpp != 4 && o.Bpp != 8 && o.Bpp != 12 {
		panic(fmt.Sprintf("Wrong specification: bits per pixel: %v!", o.Bpp))
	}

	if pm, _ := in.(*image.Paletted); pm != nil {
		if o.Bpp > 8 {
			panic("Expected color mapped image!")
		}

		bpp := util.GetDepth(pm.Pix)
		if o.LimitBpp {
			bpp = min(o.Bpp, bpp)
		}

		if o.Width != cfg.Width || o.Height != cfg.Height || o.Bpp < bpp {
			got := fmt.Sprintf("%vx%vx%v", o.Width, o.Height, o.Bpp)
			exp := fmt.Sprintf("%vx%vx%v", cfg.Width, cfg.Height, bpp)
			panic(fmt.Sprintf("image size is wrong: expected %q, got %q", exp, got))
		}

		o.Stride = o.Width
		var data []uint16

		if o.Bpp == 4 {
			o.Type = "PM_CMAP4"
			data = chunky4(pm, pm.Pix, o.Width, o.Height)
			o.Stride = (o.Width + 1) / 2
		} else {
			o.Type = "PM_CMAP8"
			var pix16 = make([]uint16, 0, len(pm.Pix))
			for _, v := range pm.Pix {
				pix16 = append(pix16, uint16(v))
			}
			data = pix16
		}

		for i := 0; i < o.Stride*o.Height; i += o.Stride {
			row := []string{}
			for _, v := range data[i : i+o.Stride] {
				o := fmt.Sprintf("0x%02x", v)
				row = append(row, o)
			}
			o.PixData = append(o.PixData, strings.Join(row, ", "))
		}
		o.Size = o.Stride * o.Height
	}
	if rgbm, _ := in.(*image.RGBA); rgbm != nil {
		o.IsRGB = true
		if o.Bpp <= 8 {
			panic("Expected RGB true color image!")
		}

		if o.Width != cfg.Width || o.Height != cfg.Height {
			got := fmt.Sprintf("%vx%vx%v", o.Width, o.Height, o.Bpp)
			exp := fmt.Sprintf("%vx%vx%v", cfg.Width, cfg.Height, o.Bpp)
			panic(fmt.Sprintf("image size is wrong: expected %q, got %q", exp, got))
		}
		o.Size = o.Width * o.Height
		o.Type = "PM_RGB12"
		o.Stride = o.Width
		dataRGB := rgb12(*rgbm, o.Height, o.Width)

		for i := 0; i < o.Stride*o.Height; i += o.Stride {
			row := []string{}
			for _, v := range dataRGB[i : i+o.Stride] {
				o := fmt.Sprintf("0x%04x", v)
				row = append(row, o)
			}
			o.PixData = append(o.PixData, strings.Join(row, ", "))
		}
	}

	var buf strings.Builder
	err = out.Execute(&buf, o)
	if err != nil {
		panic(err)
	}

	return buf.String()
}

func chunky4(im *image.Paletted, pix []uint8, width, height int) (out []uint16) {
	for y := 0; y < height; y++ {
		for x := 0; x < ((width + 1) & ^1); x += 2 {
			var x0 uint8 = im.ColorIndexAt(x, y) & 15
			var x1 uint8
			if x+1 < width {
				x1 = im.ColorIndexAt(x+1, y) & 15
			} else {
				x1 = 0
			}
			out = append(out, uint16((x0<<4)|x1))
		}
	}
	return out
}

func rgb12(img image.RGBA, height, width int) (out []uint16) {
	for y := 0; y < height; y++ {
		for x := 0; x < width; x++ {
			o := toRGB12(img.At(x, y))
			out = append(out, o)
		}
	}
	return out
}

func toRGB12(rgb color.Color) uint16 {
	r, g, b, _ := rgb.RGBA()
	var rr uint16 = uint16(r >> 8)
	var gg uint16 = uint16(g >> 8)
	var bb uint16 = uint16(b >> 8)

	return ((rr & 0xf0) << 4) | (gg & 0xf0) | (bb >> 4)
}

func bindParams(p map[string]any) (out Opts) {
	out.Name = p["name"].(string)
	out.Width = p["width"].(int)
	out.Height = p["height"].(int)
	out.Bpp = p["bpp"].(int)

	if v, ok := p["limit_bpp"]; ok {
		out.LimitBpp = v.(bool)
	}
	if v, ok := p["onlydata"]; ok {
		out.OnlyData = v.(bool)
	}
	if v, ok := p["displayable"]; ok {
		out.Displayable = v.(bool)
	}

	return out
}

type Opts struct {
	Name        string
	Width       int
	Height      int
	Bpp         int
	LimitBpp    bool
	Displayable bool
	OnlyData    bool
	Size        int
	Stride      int
	Type        string
	PixData     []string
	IsRGB       bool
}
