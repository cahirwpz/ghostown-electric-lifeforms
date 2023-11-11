#define {{ .Name }}_count {{ .Count }}

static const __data PaletteT {{ .Name }} = {
  .count = {{ .Count }},
  .colors = {
    {{ range .Colors }}
      {{- . -}},
    {{ end -}}
  }
};
