#!/usr/bin/perl
# convert QUALITY.md to html as GFM
# with relative links to the .out files

my $md = shift;
my $new = $md;
open $f, $md or
  die "input file \"$md\" not found";
my $tmp = "_tmp.md";
open $out, ">", $tmp or
  die "Could not write temp file \"$tmp\" $!";
while (<$f>) {
  if (/[\s\|](\S+\.out)/ && -f "dieharder/$1") {
    s{([\s\|])(\S+\.out)}{$1\[$2\](dieharder/$2)};
  }
  print $out $_;
}
close $out;
close $f;
$new =~ s/\.md$/.html/;
system("github-markdown -t \"dieharder - Quality and speed of the tested rng functions\" $tmp >$new");
unlink $tmp;
