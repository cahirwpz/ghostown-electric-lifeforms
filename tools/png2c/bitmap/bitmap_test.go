package bitmap

import (
	_ "embed"
	"image"
	"testing"

	"ghostown.pl/png2c/util"
)

//go:embed test/basic_bitmap.png
var basicBitmap []byte

//go:embed test/goldenfiles/basic_bitmap.txt
var basicBitmapGF string

//go:embed test/complex_bitmap.png
var complexBitmap []byte

//go:embed test/goldenfiles/complex_bitmap.txt
var complexBitmapGF string

func TestMake(t *testing.T) {

	cases := []struct {
		name       string
		inputFile  []byte
		goldenFile string
		opts       map[string]any
	}{
		{
			name:       "Basic bitmap with default flags",
			inputFile:  basicBitmap,
			goldenFile: basicBitmapGF,
			opts: map[string]any{
				"name":   "turmite_credits_1",
				"width":  256,
				"height": 256,
				"depth":  1,
			},
		},
		{
			name:       "Bitmap with non-default flags",
			inputFile:  complexBitmap,
			goldenFile: complexBitmapGF,
			opts: map[string]any{
				"name":        "stripes",
				"width":       128,
				"height":      64,
				"depth":       2,
				"limit_depth": true,
				"interleaved": true,
				"cpu_only":    true,
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
				// err := os.WriteFile("complex_out.txt", []byte(out), 777)
				// if err != nil {
				// 	panic(fmt.Sprintf("Failed to write file %q", flag.Arg(0)))
				// }
				t.Fatalf("files are different")
			}
		})
	}
}
