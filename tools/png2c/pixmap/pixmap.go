package pixmap

import (
	_ "embed"
	"fmt"
	"image"
	"image/color"
	"strings"

	"ghostown.pl/png2c/util"
)

//go:embed template.tpl
var tpl string

func Make(in image.Image, cfg image.Config, opts map[string]any) string {
	o := bindParams(opts)

	// Validate bpp
	if o.Bpp != 4 && o.Bpp != 8 && o.Bpp != 12 {
		panic(fmt.Sprintf("Wrong specification: bits per pixel: %v!", o.Bpp))
	}

	// Handle RGB images
	if rgbm, _ := in.(*image.RGBA); rgbm != nil {
		o.IsRGB = true
		if o.Bpp <= 8 {
			panic("Expected RGB true color image!")
		}

		// Validate image' size
		if o.Width != cfg.Width || o.Height != cfg.Height {
			got := fmt.Sprintf("%vx%vx%v", o.Width, o.Height, o.Bpp)
			exp := fmt.Sprintf("%vx%vx%v", cfg.Width, cfg.Height, o.Bpp)
			panic(fmt.Sprintf("image size is wrong: expected %q, got %q", exp, got))
		}

		// Calculate the data
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
		// Handle paletted and grayscale images
	} else {
		var pix []uint8
		if pm, _ := in.(*image.Paletted); pm != nil {
			pix = pm.Pix
		} else if gray, _ := in.(*image.Gray); gray != nil {
			pix = gray.Pix
		} else {
			panic("Expected color mapped or grayscale image!")
		}

		// Set and validate bpp
		if o.Bpp > 8 {
			panic("Depth too big!")
		}
		bpp := util.GetDepth(pix)
		if o.LimitBpp {
			bpp = min(o.Bpp, bpp)
		}

		// Validate image' size
		if o.Width != cfg.Width || o.Height != cfg.Height || o.Bpp < bpp {
			got := fmt.Sprintf("%vx%vx%v", o.Width, o.Height, o.Bpp)
			exp := fmt.Sprintf("%vx%vx%v", cfg.Width, cfg.Height, bpp)
			panic(fmt.Sprintf("image size is wrong: expected %q, got %q", exp, got))
		}

		// Calculate the data
		o.Stride = o.Width
		var data []uint16
		if o.Bpp == 4 {
			o.Type = "PM_CMAP4"
			data = chunky4(in, pix, o.Width, o.Height)
			o.Stride = (o.Width + 1) / 2
		} else {
			o.Type = "PM_CMAP8"
			var pix16 = make([]uint16, 0, len(pix))
			for _, v := range pix {
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

	out := util.CompileTemplate(tpl, o)

	return out
}

func chunky4(im image.Image, pix []uint8, width, height int) (out []uint16) {
	for y := 0; y < height; y++ {
		for x := 0; x < ((width + 1) & ^1); x += 2 {
			var x0, x1 uint8
			if gray, _ := im.(*image.Gray); gray != nil {
				x0 = gray.GrayAt(x, y).Y & 15
				x1 = gray.GrayAt(x+1, y).Y & 15
			} else if pm, _ := im.(*image.Paletted); pm != nil {
				x0 = pm.ColorIndexAt(x, y) & 15
				x1 = pm.ColorIndexAt(x+1, y) & 15
			}
			if x+1 >= width {
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
	// Template-specific data
	Size    int
	Stride  int
	Type    string
	PixData []string
	IsRGB   bool
}
