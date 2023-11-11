package params

import (
	"fmt"
	"strconv"
	"strings"
)

type Param struct {
	Name     string
	CastType int
	Value    any
}

const (
	TYPE_STRING int = 1
	TYPE_INT    int = 2
	TYPE_BOOL   int = 3
)

func ParseOpts(s string, params ...Param) map[string]any {
	var mandatory []Param
	var optional []Param
	result := map[string]any{}

	for _, v := range params {
		if v.Value == nil {
			mandatory = append(mandatory, v)
		} else {
			optional = append(optional, v)
		}
	}

	ss := strings.Split(s, ",")

	// parse mandatory arguments
	for _, v := range mandatory {
		if len(ss) == 0 {
			panic(fmt.Sprintf("Missing argument %q!", v.Name))
		}
		v.Value = ss[0]
		result = cast(v, result)
		ss = ss[1:]
	}

	// parse optional arguments
	for _, o := range optional {
		var p Param
		var found bool
		for _, s := range ss {
			if strings.Contains(s, o.Name) {
				if strings.HasPrefix(s, "+") || strings.HasPrefix(s, "-") {
					p.Name = s[1:]
					p.Value = string(s[0]) == "+"
					p.CastType = o.CastType
				} else if strings.Contains(s, "=") {
					e := strings.Split(s, "=")
					if len(e) != 2 {
						panic("Malformed optional argument")
					}
					p.Name = e[0]
					p.Value = e[1]
					p.CastType = o.CastType

				}
				result = cast(p, result)
				found = true
				break
			}
		}
		if !found {
			result = cast(o, result)
		}
	}

	return result
}

func cast(p Param, out map[string]any) map[string]any {
	if p.CastType != TYPE_BOOL {
		names := strings.Split(p.Name, ",")
		values := strings.Split(p.Value.(string), "x")
		if len(values) == len(names) {
			for i, name := range names {
				var nv any
				switch p.CastType {
				case TYPE_STRING:
					nv = values[i]
				case TYPE_INT:
					v, err := strconv.Atoi(values[i])
					if err != nil {
						panic(err)
					}
					nv = v
				}
				out[name] = nv
			}
		} else if len(names) == 1 && len(values) > 1 {
			vs := []int{}
			for _, v := range values {
				ve, err := strconv.Atoi(v)
				if err != nil {
					panic(err)
				}
				vs = append(vs, ve)
			}
			out[names[0]] = vs
		}
	} else {
		out[p.Name] = p.Value
	}

	return out
}
