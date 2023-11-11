package palette

import (
	_ "embed"
	"image"
	"testing"

	"ghostown.pl/png2c/util"
)

//go:embed test/basic_pal.png
var basicPal []byte

//go:embed test/goldenfiles/basic_pal.txt
var basicPalGF string

func TestMake(t *testing.T) {

	cases := []struct {
		name       string
		inputFile  []byte
		goldenFile string
		opts       map[string]any
	}{
		{
			name:       "Basic palette",
			inputFile:  basicPal,
			goldenFile: basicPalGF,
			opts: map[string]any{
				"name":  "tree_pal_electric",
				"count": 32,
			},
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			img, cfg, err := util.DecodePNG(tt.inputFile)
			if err != nil {
				t.Fatalf("failed to decode input file: %v", err)
			}
			pm, _ := img.(*image.Paletted)
			if pm == nil {
				t.Fatalf("only paletted images are supported")
			}
			out := Make(pm, cfg, tt.opts)

			if out != tt.goldenFile {
				// err := os.WriteFile("basic_out.txt", []byte(out), 777)
				// if err != nil {
				// 	panic(fmt.Sprintf("Failed to write file %q", flag.Arg(0)))
				// }
				t.Fatalf("files are different")
			}
		})
	}
}
