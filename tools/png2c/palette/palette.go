package palette

import (
	_ "embed"
	"fmt"
	"html/template"
	"image"
	"strings"
)

//go:embed template.tpl
var tpl string

type Palette struct {
	Name   string
	Count  int
	Shared bool
	Colors []string
}

func Make(in *image.Paletted, cfg image.Config, opts map[string]any) string {
	var out Palette
	name := opts["name"].(string)
	count := opts["count"].(int)
	if v, ok := opts["shared"]; ok {
		out.Shared = v.(bool)
	}

	tmpl, err := template.New("pal_template").Parse(tpl)
	if err != nil {
		panic(err)
	}

	p := in.Palette
	// p := util.CleanPalette(in.Palette)
	if len(p) > count {
		msg := fmt.Sprintf("Expected max %v colors, got %v", count, len(p))
		panic(msg)
	}

	for _, v := range p {
		r, g, b, _ := v.RGBA()
		c := fmt.Sprintf("0x%x%x%x", r>>8/16, g>>8/16, b>>8/16)
		out.Colors = append(out.Colors, c)
	}

	out.Name = name
	out.Count = len(p)

	var buf strings.Builder
	err = tmpl.Execute(&buf, out)
	if err != nil {
		panic(err)
	}

	return buf.String()
}
