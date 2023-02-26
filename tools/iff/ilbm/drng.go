package ilbm

import (
	"fmt"
	"ghostown.pl/iff"
)

type DColor struct {
	cell  uint8
	r     uint8
	g     uint8
	b     uint8
}

type DIndex struct {
	cell  uint8
	index uint8
}

type DRNG struct {
	min   uint8
	max   uint8
	rate  int16
	flags int16
	dcolor []DColor
	dindex []DIndex
}

func (drng DRNG) Name() string {
	return "DRNG"
}

func (drng *DRNG) Read(r iff.Reader) {
	drng.min = r.ReadU8()
	drng.max = r.ReadU8()
	drng.rate = r.ReadI16()
	drng.flags = r.ReadI16()
	ntrue := r.ReadU8()
	nregs := r.ReadU8()

	drng.dcolor = make([]DColor, ntrue, ntrue)
	for i := 0; uint8(i) < ntrue; i++ {
		drng.dcolor[i].cell = r.ReadU8()
		drng.dcolor[i].r = r.ReadU8()
		drng.dcolor[i].g = r.ReadU8()
		drng.dcolor[i].b = r.ReadU8()
	}

	drng.dindex = make([]DIndex, nregs, nregs)
	for i := 0; uint8(i) < nregs; i++ {
		drng.dindex[i].cell = r.ReadU8()
		drng.dindex[i].index = r.ReadU8()
	}
}

func (drng DRNG) String() string {
	s := "{"

	var flags string
	if drng.flags == 0x1 {
		flags = "RNG_ACTIVE"
	} else {
		flags = "NONE"
	}

	s += fmt.Sprintf("min: %d, max: %d, ", drng.min, drng.max)
	s += fmt.Sprintf("rate: %d, flags = %s, ", drng.rate, flags)

	sep := ""
	s += "DColor: {"
	for _, dc := range drng.dcolor {
		s += sep
		s += fmt.Sprintf("%d: #%02x%02x%02x", dc.cell, dc.r, dc.g, dc.b)
		sep = ", "
	}
	s += "}, "

	sep = ""
	s += "DIndex: {"
	for _, di := range drng.dindex {
		s += sep
		s += fmt.Sprintf("%d: %d", di.cell, di.index)
		sep = ", "
	}
	s += "}}"

	return s
}

func makeDRNG() iff.Chunk {
	return &DRNG{}
}
