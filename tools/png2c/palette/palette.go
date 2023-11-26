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
	if !o.StoreUnused {
		p = util.CleanPalette(in.Pix, p)
	}
	// o.Count = len(p)
	if len(p) > o.Count {
		msg := fmt.Sprintf("Expected max %v colors, got %v", o.Count, len(p))
		panic(msg)
	}

	for _, v := range p {
		r, g, b, _ := v.RGBA()
		c := fmt.Sprintf("0x%x%x%x", r>>8/16, g>>8/16, b>>8/16)
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
	Colors      []string
	StoreUnused bool
}
