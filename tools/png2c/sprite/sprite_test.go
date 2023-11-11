package sprite

import (
	_ "embed"
	"image"
	"testing"

	"ghostown.pl/png2c/util"
)

//go:embed test/basic_sprite.png
var basicSprite []byte

//go:embed test/goldenfiles/basic_sprite.txt
var basicSpriteGF string

//go:embed test/attached_sprite.png
var attachedSprite []byte

//go:embed test/goldenfiles/attached_sprite.txt
var attachedSpriteGF string

func TestMake(t *testing.T) {

	cases := []struct {
		name       string
		inputFile  []byte
		goldenFile string
		opts       map[string]any
	}{
		{
			name:       "Basic sprite",
			inputFile:  basicSprite,
			goldenFile: basicSpriteGF,
			opts: map[string]any{
				"name":   "grass",
				"height": 32,
				"count":  8,
			},
		},
		{
			name:       "Attached sprite",
			inputFile:  attachedSprite,
			goldenFile: attachedSpriteGF,
			opts: map[string]any{
				"name":     "wireworld_chip",
				"height":   64,
				"count":    4,
				"attached": true,
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
				// err := os.WriteFile("attached_out.txt", []byte(out), 777)
				// if err != nil {
				// 	panic(fmt.Sprintf("Failed to write file %q", flag.Arg(0)))
				// }
				t.Fatalf("files are different")
			}
		})
	}
}
