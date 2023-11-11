package pixmap

import (
	_ "embed"
	"testing"

	"ghostown.pl/png2c/util"
)

//go:embed test/basic_pixmap.png
var basicPixmap []byte

//go:embed test/goldenfiles/basic_pixmap.txt
var basicPixmapGF string

//go:embed test/complex_pixmap.png
var complexPixmap []byte

//go:embed test/goldenfiles/complex_pixmap.txt
var complexPixmapGF string

//go:embed test/complex_pixmap_rgb.png
var basicRGBPixmap []byte

//go:embed test/goldenfiles/basic_pixmap_rgb.txt
var basicRGBPixmapGF string

func TestMake(t *testing.T) {

	cases := []struct {
		name       string
		inputFile  []byte
		goldenFile string
		opts       map[string]any
	}{
		{
			name:       "Basic paletted pixmap",
			inputFile:  basicPixmap,
			goldenFile: basicPixmapGF,
			opts: map[string]any{
				"name":   "electric_lifeforms",
				"width":  320,
				"height": 256,
				"bpp":    4,
			},
		},
		{
			name:       "Paletted pixmap with non-default flags",
			inputFile:  complexPixmap,
			goldenFile: complexPixmapGF,
			opts: map[string]any{
				"name":        "bar",
				"width":       480,
				"height":      67,
				"bpp":         4,
				"onlydata":    true,
				"displayable": true,
				"limit_bpp":   true,
			},
		},
		{
			name:       "Basic RGB pixmap",
			inputFile:  basicRGBPixmap,
			goldenFile: basicRGBPixmapGF,
			opts: map[string]any{
				"name":   "texture",
				"width":  480,
				"height": 67,
				"bpp":    12,
			},
		},
	}

	for _, tt := range cases {
		t.Run(tt.name, func(t *testing.T) {
			img, cfg, err := util.DecodePNG(tt.inputFile)
			if err != nil {
				t.Fatalf("failed to decode input file: %v", err)
			}
			out := Make(img, cfg, tt.opts)

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
