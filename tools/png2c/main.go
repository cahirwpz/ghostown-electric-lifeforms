package main

import (
	"flag"
	"fmt"
	"image"
	"io"
	"os"
	"strings"

	b "ghostown.pl/png2c/bitmap"
	p "ghostown.pl/png2c/palette"
	pms "ghostown.pl/png2c/params"
	"ghostown.pl/png2c/pixmap"
	"ghostown.pl/png2c/sprite"
	"ghostown.pl/png2c/util"
)

var (
	bitmapVar  string
	pixmapVar  string
	spriteVar  string
	paletteVar string
)

func init() {
	flag.StringVar(&bitmapVar, "bitmap", "", "Output Amiga bitmap [name,dimensions,flags]")
	flag.StringVar(&pixmapVar, "pixmap", "", "Output pixel map [name,width,height,type,flags]")
	flag.StringVar(&spriteVar, "sprite", "", "Output Amiga sprite [name]")
	flag.StringVar(&paletteVar, "palette", "", "Output Amiga palette [name,colors]")

	flag.Parse()
}

func main() {
	r, err := os.Open(flag.Arg(0))
	if err != nil {
		panic(fmt.Sprintf("Failed to open file %q", flag.Arg(0)))
	}
	file, _ := io.ReadAll(r)

	img, cfg, err := util.DecodePNG(file)
	if err != nil {
		panic(err)
	}

	var out string
	if bitmapVar != "" {
		// Check if image has a palette
		pm, _ := img.(*image.Paletted)
		if pm == nil {
			panic("only paletted images are supported")
		}

		opts := pms.ParseOpts(bitmapVar,
			pms.Param{Name: "name", CastType: pms.TYPE_STRING},
			pms.Param{Name: "width,height,depth", CastType: pms.TYPE_INT},
			pms.Param{Name: "extract_at", CastType: pms.TYPE_INT, Value: "-1x-1"},
			pms.Param{Name: "interleaved", CastType: pms.TYPE_BOOL, Value: false},
			pms.Param{Name: "cpuonly", CastType: pms.TYPE_BOOL, Value: false},
			pms.Param{Name: "shared", CastType: pms.TYPE_BOOL, Value: false},
			pms.Param{Name: "limit_depth", CastType: pms.TYPE_BOOL, Value: false},
			pms.Param{Name: "onlydata", CastType: pms.TYPE_BOOL, Value: false},
		)

		out += b.Make(pm, cfg, opts)
	}

	if paletteVar != "" {
		// Check if image has a palette
		pm, _ := img.(*image.Paletted)
		if pm == nil {
			panic("only paletted images are supported")
		}
		opts := pms.ParseOpts(paletteVar,
			pms.Param{Name: p.OPT_NAME, CastType: pms.TYPE_STRING},
			pms.Param{Name: p.OPT_COUNT, CastType: pms.TYPE_INT},
		)

		out += p.Make(pm, cfg, opts)
	}

	if pixmapVar != "" {
		opts := pms.ParseOpts(pixmapVar,
			pms.Param{Name: "name", CastType: pms.TYPE_STRING},
			pms.Param{Name: "width,height,bpp", CastType: pms.TYPE_INT},
			pms.Param{Name: "limit_bpp", CastType: pms.TYPE_BOOL, Value: false},
			pms.Param{Name: "displayable", CastType: pms.TYPE_BOOL, Value: false},
			pms.Param{Name: "onlydata", CastType: pms.TYPE_BOOL, Value: false},
		)
		out += pixmap.Make(img, cfg, opts)
	}

	if spriteVar != "" {
		pm, _ := img.(*image.Paletted)
		if pm == nil {
			panic("only paletted images are supported")
		}
		opts := pms.ParseOpts(spriteVar,
			pms.Param{Name: "name", CastType: pms.TYPE_STRING},
			pms.Param{Name: "height", CastType: pms.TYPE_INT},
			pms.Param{Name: "count", CastType: pms.TYPE_INT},
			pms.Param{Name: "attached", CastType: pms.TYPE_BOOL, Value: false},
		)
		out += sprite.Make(pm, cfg, opts)
	}

	inName := strings.Split(r.Name(), ".")[0]
	outName := fmt.Sprintf("%s.c", inName)
	err = os.WriteFile(outName, []byte(out), 777)
	if err != nil {
		panic(fmt.Sprintf("Failed to write file %q", flag.Arg(0)))
	}
}
