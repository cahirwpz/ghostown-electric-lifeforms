package palette

import (
	_ "embed"
	"fmt"
	"html/template"
	"image"
	"strings"

	"ghostown.pl/png2c/util"
)

//go:embed template.tpl
var tpl string

func Make(in *image.Paletted, cfg image.Config, opts map[string]any) string {
	o := bindParams(opts)
	tmpl, err := template.New("pal_template").Parse(tpl)
	if err != nil {
		panic(err)
	}

	p := in.Palette
	if o.StoreUnused {
		// Trim colors that are not part of the palette
		p = in.Palette[0:o.Count]
	} else {
		p = util.CleanPalette(in.Pix, p)
	}

	if len(p) > o.Count {
		msg := fmt.Sprintf("Expected max %v colors, got %v", o.Count, len(p))
		panic(msg)
	}

	for _, v := range p {
		r, g, b, _ := v.RGBA()
		c := (r>>8/16)<<8 | (g>>8/16)<<4 | (b >> 8 / 16)
		o.Colors = append(o.Colors, c)
	}

	var buf strings.Builder
	err = tmpl.Execute(&buf, o)
	if err != nil {
		panic(err)
	}

	return buf.String()
}

func bindParams(p map[string]any) (out Opts) {
	out.Name = p["name"].(string)
	out.Count = p["count"].(int)

	if v, ok := p["shared"]; ok {
		out.Shared = v.(bool)
	}
	if v, ok := p["store_unused"]; ok {
		out.StoreUnused = v.(bool)
	}

	return out
}

type Opts struct {
	Name        string
	Count       int
	Shared      bool
	StoreUnused bool
	// Template-specific data.
	Colors []uint32
}
