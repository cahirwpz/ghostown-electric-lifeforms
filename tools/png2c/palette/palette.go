package palette

import (
	_ "embed"
	"fmt"
	"image"
	"log"

	"ghostown.pl/png2c/util"
)

//go:embed template.tpl
var tpl string

func Make(in *image.Paletted, cfg image.Config, opts map[string]any) string {
	o := bindParams(opts)

	// Clean up the palette
	p := in.Palette
	if o.StoreUnused {
		p = in.Palette[0:o.Count]
	} else {
		p = util.CleanPalette(in.Pix, p)
	}

	if len(p) > o.Count {
		log.Panicf("Expected max %v colors, got %v", o.Count, len(p))
	}

	// Calculate the color data
	for _, v := range p {
		c := util.RGB12(v)
		o.ColorsData = append(o.ColorsData, fmt.Sprintf("0x%03x", c))
	}

	// Compile the template
	out := util.CompileTemplate(tpl, o)

	return out
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
	ColorsData []string
}
