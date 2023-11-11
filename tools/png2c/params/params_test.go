package params

import (
	"reflect"
	"testing"
)

func Test_ParseOpts(t *testing.T) {

	cases := []struct {
		desc      string
		input     string
		params    []Param
		expOutput map[string]any
	}{
		{
			desc:  "default values for optional parameters",
			input: "fruit,16x16x2",
			params: []Param{
				{Name: "name", CastType: TYPE_STRING},
				{Name: "width,height,depth", CastType: TYPE_INT},
				{Name: "onlydata", CastType: TYPE_BOOL, Value: false},
			},
			expOutput: map[string]any{
				"name":     "fruit",
				"width":    16,
				"height":   16,
				"depth":    2,
				"onlydata": false,
			},
		},
		{
			desc:  "with optional bool flags override",
			input: "fruit,8x8x3,+defaultFalse,-defaultTrue",
			params: []Param{
				{Name: "name", CastType: TYPE_STRING},
				{Name: "width,height,depth", CastType: TYPE_INT},
				{Name: "defaultFalse", CastType: TYPE_BOOL, Value: false},
				{Name: "defaultTrue", CastType: TYPE_BOOL, Value: true},
			},
			expOutput: map[string]any{
				"name":         "fruit",
				"width":        8,
				"height":       8,
				"depth":        3,
				"defaultFalse": true,
				"defaultTrue":  false,
			},
		},
		{
			desc:  "with optional parameter override",
			input: "fruit,8x8x3,extract_at=10x108",
			params: []Param{
				{Name: "name", CastType: TYPE_STRING},
				{Name: "width,height,depth", CastType: TYPE_INT},
				{Name: "extract_at", CastType: TYPE_INT, Value: "0x0"},
			},
			expOutput: map[string]any{
				"name":       "fruit",
				"width":      8,
				"height":     8,
				"depth":      3,
				"extract_at": []any{10, 108},
			},
		},
		{
			desc:  "with optional parameter",
			input: "fruit,8x8x3",
			params: []Param{
				{Name: "name", CastType: TYPE_STRING},
				{Name: "width,height,depth", CastType: TYPE_INT},
				{Name: "extract_at", CastType: TYPE_INT, Value: "10x10"},
			},
			expOutput: map[string]any{
				"name":       "fruit",
				"width":      8,
				"height":     8,
				"depth":      3,
				"extract_at": []any{10, 10},
			},
		},
	}

	for _, tc := range cases {
		out := ParseOpts(tc.input, tc.params...)
		for expKey, expValue := range tc.expOutput {
			value, ok := out[expKey]
			if !ok {
				t.Errorf("Expected key %q is missing!", expKey)
			}
			if reflect.TypeOf(expValue).Kind() == reflect.Slice {
				for i, v := range expValue.([]any) {
					eh := value.([]any)
					if v != eh[i] {
						t.Errorf("For key %q expected value is %q, got %q", expKey, v, eh[i])
					}
				}
			} else if value != expValue {
				t.Errorf("For key %q expected value is %q, got %q", expKey, expValue, value)
			}
		}
	}
}
