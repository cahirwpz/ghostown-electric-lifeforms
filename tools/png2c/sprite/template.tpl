#define {{.Name}}_height {{.Height}}
#define {{.Name}}_sprites {{.Count}}
{{ range .Sprites }}
static __data_chip SprDataT {{.Name}}_sprdat = {
  .pos = SPRPOS(0, 0),
  .ctl = SPRCTL(0, 0, {{.Attached}}, {{.Height}}),
  .data = {
      {{ range .Data -}}
			  {{ . }}
      {{ end -}}
	}
};
{{ end }}
static __data SpriteT {{.Name}}[{{.Count}}] = {
  {{ range .Sprites -}}
  {
    .sprdat = &{{.Name}}_sprdat,
    .height = {{.Height}},
    .attached = {{.Attached}},
  },
  {{end }}
};
