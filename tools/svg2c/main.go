package main

import (
	"bytes"
	"flag"
	"fmt"
	"html/template"
	"log"
	"math"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"

	"github.com/joshvarga/svgparser"
)

const (
	svgTemplate = `// {{ .Name }}
static GreetsT gr{{ .Name }} = {
  .curr = NULL,
  .n = 0,
  .x = 0,
  .y = 0,
  .origin_x = {{ .Origin.X }},
  .origin_y = {{ .Origin.Y }},
  .delay = 0,
  .data = {
    {{- range .Data }}
    {{ .Lenght }},
    {{- range .DeltaPoints }}{{ .X }},{{.Y}},{{- end }}
    {{- end }}
    -1
  }
};
`
)

type SvgData struct {
	Data   []GeometricData
	Origin Point
	Name   string
}

type GeometricData struct {
	Length      int
	Points      []Point
	DeltaPoints []Point
}

type Point struct {
	X int
	Y int
}

func Add(p1, p2 Point) (p Point) {
	return Point{p1.X + p2.X, p1.Y + p2.Y}
}

func Sub(p1, p2 Point) (p Point) {
	return Point{p1.X - p2.X, p1.Y - p2.Y}
}

func parseCoordinate(coord string) (int, error) {
	num, err := strconv.ParseFloat(coord, 32)
	if err != nil {
		return 0, err
	}
	return int(math.Round(num)), nil
}

func parseLine(el *svgparser.Element) []Point {
	X1, err := parseCoordinate(el.Attributes["x1"])
	if err != nil {
		log.Fatalf("Error parsing line coord x1 - value: %s\n",
			el.Attributes["x1"])
	}
	Y1, err := parseCoordinate(el.Attributes["y1"])
	if err != nil {
		log.Fatalf("Error parsing coordinates y1 - value: %s\n",
			el.Attributes["y1"])
	}
	X2, err := parseCoordinate(el.Attributes["x2"])
	if err != nil {
		log.Fatalf("Error parsing coordinates x2 - value: %s\n",
			el.Attributes["x2"])
	}
	Y2, err := parseCoordinate(el.Attributes["y2"])
	if err != nil {
		log.Fatalf("Error parsing coordinates y2 - value: %s\n",
			el.Attributes["y2"])
	}
	return []Point{{X1, Y1}, {X2, Y2}}
}

func parsePolyLine(el *svgparser.Element) []Point {
	pointsString := el.Attributes["points"]
	space := regexp.MustCompile(`\s+`)
	trimmedPoints := space.ReplaceAllString(pointsString, " ")
	points := strings.Split(trimmedPoints, " ")
        var parsedPoints = make([]Point, 0, len(points))
	for _, point := range points {
		if point == "" {
			continue
		}
		coords := strings.Split(point, ",")
		x, err := parseCoordinate(strings.TrimSpace(coords[0]))
		if err != nil {
			log.Fatalf("Error parsing coordinate x in point: '%v'\n", point)
		}
		y, err := parseCoordinate(coords[1])
		if err != nil {
			log.Fatalf("Error parsing coordinate y in point: '%v'\n", point)
		}
		parsedPoint := Point{x, y}
		parsedPoints = append(parsedPoints, parsedPoint)
	}
	return parsedPoints
}

func (sd *SvgData) CalcOrigin() {
	minPoint := sd.GetMinPoint()
	maxPoint := sd.GetMaxPoint()
	midPoint := Point{
		(maxPoint.X - minPoint.X) / 2,
		(maxPoint.Y - minPoint.Y) / 2,
	}
	sd.Origin = Add(minPoint, midPoint)
}

func (sd *SvgData) CalcDataWithOffset() {
	for idx, gData := range sd.Data {
		sd.Data[idx].DeltaPoints = gData.GetWithOffsetAndDelta(sd.Origin)
	}
}

func (sd *SvgData) GetMinPoint() Point {
	var minPoint = sd.Data[0].Points[0]
	for _, gData := range sd.Data {
		for _, point := range gData.Points {
			if point.X < minPoint.X {
				minPoint.X = point.X
			}
			if point.Y < minPoint.Y {
				minPoint.Y = point.Y
			}
		}
	}
	return minPoint
}

func (sd *SvgData) GetMaxPoint() Point {
	var maxPoint = sd.Data[0].Points[0]
	for _, gData := range sd.Data {
		for _, point := range gData.Points {
			if point.X > maxPoint.X {
				maxPoint.X = point.X
			}
			if point.Y > maxPoint.Y {
				maxPoint.Y = point.Y
			}
		}
	}
	return maxPoint
}

func (sd *SvgData) Export() (output string, err error) {
	t, err := template.New("svg").Parse(svgTemplate)
	if err != nil {
		return
	}

	file, err := os.Create(outputPath)
	if err != nil {
		return "", err
	}
	defer file.Close()

	var data bytes.Buffer
	err = t.Execute(&data, sd)
	if err != nil {
		return "", err
	}
	return data.String(), nil
}

func (gd *GeometricData) GetWithOffsetAndDelta(offset Point) []Point {
	inputPoints := gd.GetPointsWithOffset(offset)
	deltaPoints := []Point{inputPoints[0]}
	for index, point := range inputPoints {
		if index != 0 {
			lastPoint := inputPoints[index-1]
			deltaPoints = append(deltaPoints, Sub(point, lastPoint))
		}
	}
	return deltaPoints
}

func (gd *GeometricData) GetPointsWithOffset(offset Point) []Point {
	var withOffset []Point
	for _, point := range gd.Points {
		withOffset = append(withOffset, Sub(point, offset))
	}
	return withOffset
}

func handleFile(file *os.File, name string) string {
	svgData := SvgData{[]GeometricData{}, Point{0, 0}, name}

	element, _ := svgparser.Parse(file, false)
	elements := element.Children

	for _, element := range elements {
		elementType := element.Name
		switch elementType {
		case "polyline":
			parsedPoints := parsePolyLine(element)
			svgData.Data = append(svgData.Data,
				GeometricData{len(parsedPoints), parsedPoints, []Point{}})
		case "line":
			parsedPoints := parseLine(element)
			svgData.Data = append(svgData.Data,
				GeometricData{len(parsedPoints), parsedPoints, []Point{}})
		case "path":
			if verbose {
				fmt.Println("Found path element, geometry won't be parsed")
			}
		default:
			if verbose {
				fmt.Printf("[WARN] Parsed element %s\n", element.Name)
			}
		}
	}
	svgData.CalcOrigin()
	svgData.CalcDataWithOffset()
	data, err := svgData.Export()
	if err != nil {
		log.Fatal(err)
	}
	return data
}

const defaultFileName = "data.c"

var outputPath string
var verbose bool
var printHelp bool

func init() {
	flag.StringVar(&outputPath, "o", defaultFileName, "Output filename with processed data")
	flag.BoolVar(&verbose, "v", false, "Output verbose logs")
	flag.BoolVar(&printHelp, "help", false, "Prints this message")
}

func main() {

	flag.Parse()

	if printHelp || len(flag.Args()) < 1 || len(outputPath) == 0 {
		flag.PrintDefaults()
		os.Exit(1)
	}

	var exportString string

	filesToConvert := flag.Args()

	for _, entry := range filesToConvert {
		file, err := os.Open(entry)
		if err != nil {
			log.Fatal(err)
		}
		defer file.Close()
		fileExt := filepath.Ext(entry)
		fileName := filepath.Base(entry)
		if fileExt == ".svg" {
			if verbose {
				fmt.Println("Converting file:", fileName)
			}
			exportString += handleFile(file, strings.TrimSuffix(fileName, fileExt))
		}
	}

	if len(exportString) == 0 {
		os.Exit(0)
	}

	file, err := os.Create(outputPath)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()
	file.WriteString(exportString)
	if err != nil {
		log.Fatal(err)
	}
}
