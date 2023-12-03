open Sat

let blit_n = int_of_string Sys.argv.(1)
let survive = if Array.length Sys.argv > 3 then Sys.argv.(3) else "23"
let born    = if Array.length Sys.argv > 4 then Sys.argv.(4) else "3"

let lut_a1 = lut_create 2
let lut_a2 = lut_create 2
let lut_b1 = lut_create 2
let lut_b2 = lut_create 2

let luts = Array.init blit_n (fun _ -> lut_create 3)

let sels = Array.init blit_n (fun i ->
  let n = 5 + i in
  (one_hot_sel n, one_hot_sel n, one_hot_sel n))

let inp = input 5

let list_last xs =
  List.nth xs (List.length xs - 1)

let rec build dat layer_i =
  if layer_i = blit_n then List.map list_last dat
  else
    let (sa, sb, sc) = sels.(layer_i) in
    let dat =
      dat |> List.map (fun dat_vec ->
        let xa = one_hot_mux sa dat_vec in
        let xb = one_hot_mux sb dat_vec in
        let xc = one_hot_mux sc dat_vec in
        dat_vec @ [ lut_lookup luts.(layer_i) [ xa; xb; xc ] ])
    in build dat (layer_i + 1)

let pre = inp |> List.map (fun (data, _) ->
  match data with
  | [ a0; a1; b0; b1; x ] ->
    let v1 = lut_lookup lut_a1 [ a0; a1 ] in
    let v2 = lut_lookup lut_a2 [ a0; a1 ] in
    let v3 = lut_lookup lut_b1 [ b0; b1 ] in
    let v4 = lut_lookup lut_b2 [ b0; b1 ] in
    [ v1; v2; v3; v4; x ]
  | _ -> assert false)

let outp = build pre 0

let () = outp |> List.iteri (fun i x ->
  let n = (i land 3) + ((i lsr 1) land 6) in
  let b =
    if (i land 0x10) > 0 then
      String.contains survive (Char.chr (Char.code '0' + n - 1))
    else
      String.contains born (Char.chr (Char.code '0' + n))
  in
  if b then set_true x else set_false x)

let run_at i =
  let v = ref (i land 0x10) in
  v := !v lor (lut_lookup_n lut_a1 (i land 0x3) lsl 0);
  v := !v lor (lut_lookup_n lut_a2 (i land 0x3) lsl 1);
  v := !v lor (lut_lookup_n lut_b1 ((i lsr 2) land 0x3) lsl 2);
  v := !v lor (lut_lookup_n lut_b2 ((i lsr 2) land 0x3) lsl 3);
  for i = 0 to blit_n - 1 do
    let (sa, sb, sc) = sels.(i) in
    let b0 = (!v lsr (one_hot_sel_n sa)) land 1 in
    let b1 = (!v lsr (one_hot_sel_n sb)) land 1 in
    let b2 = (!v lsr (one_hot_sel_n sc)) land 1 in
    v := !v lor (lut_lookup_n luts.(i) (b2*4 + b1*2 + b0) lsl (i + 5))
  done;
  !v

let () = 
  if Sys.argv.(2) = "-o" then dump_dimacs ()
  else if Sys.argv.(2) = "-d" then begin
    read_dimacs ();
    Printf.printf "X0 = LUT (number of set bits in Lo):\n";
    dump_lut lut_a1;
    Printf.printf "X1 = LUT (number of set bits in Lo):\n";
    dump_lut lut_a2;
    Printf.printf "X2 = LUT (number of set bits in Hi):\n";
    dump_lut lut_b1;
    Printf.printf "X3 = LUT (number of set bits in Hi):\n";
    dump_lut lut_b2;
    Printf.printf "X4 = (previous bit set)\n";
    for i = 0 to blit_n - 1 do
      let (sa, sb, sc) = sels.(i) in
      Printf.printf "X%d LUT (X%d, X%d, X%d):\n"
        (i + 5)
        (one_hot_sel_n sc)
        (one_hot_sel_n sb)
        (one_hot_sel_n sa);
      dump_lut luts.(i)
    done
  end else if Sys.argv.(2) = "-t" then begin
    read_dimacs ();
    for i = 0 to 31 do
      let n = (i land 3) + ((i lsr 1) land 6) in
      let v = run_at i in
      Printf.printf "%s + %d -> %d%d%d%d_%d_%d%d%d\n"
        (if (i land 0x10) > 0 then "alive" else "dead ")
        n
        ((v lsr 0) land 1)
        ((v lsr 1) land 1)
        ((v lsr 2) land 1)
        ((v lsr 3) land 1)
        ((v lsr 4) land 1)
        ((v lsr 5) land 1)
        ((v lsr 6) land 1)
        ((v lsr 7) land 1)
    done
  end else begin
    Printf.eprintf "Bad flags\n";
    exit 1
  end
