static {{ if not .CpuOnly }}__data_chip{{ end }} u_short _{{ .Name }}_bpl[] = {
	{{ range .BplData }}
      {{- . -}},
    {{ end -}}
};

#define {{ .Name }}_width {{ .Width }}
#define {{ .Name }}_height {{ .Height }}
#define {{ .Name }}_depth {{ .Depth }}
#define {{ .Name }}_bytesPerRow {{ .BytesPerRow }}
#define {{ .Name }}_bplSize {{ .BplSize }}
#define {{ .Name }}_size {{ .Size}}
{{ if not .OnlyData }}
const {{if .Shared }} static {{ end }} __data BitmapT {{ .Name }} = {
	.width = {{ .Width }},
	.height = {{ .Height }},
	.depth = {{ .Depth }},
	.bytesPerRow = {{ .BytesPerRow }},
	.bplSize = {{ .BplSize }},
	.flags = {{ .Flags }}
	.planes = {
		{{ range .BplPointers }}
			{{- . -}},
		{{ end }}
	},
};
{{ end }}