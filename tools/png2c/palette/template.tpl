#define {{ .Name }}_count {{ .Count }}

{{if not .Shared }} static {{ end }} const __data PaletteT {{ .Name }} = {
  .count = {{ .Count }},
  .colors = {
    {{ range .Colors }}
      {{- . -}},
    {{ end -}}
  }
};
