# toneGenerator
Introduction
============
A simple tool for generating sound frequencies. The resulting data is in the wave file format and it can be exported to a file.
If no file is specified, the resulting data is written to the standard output, allowing to use the data on the fly.

For example, using the shell functionalities we could listen the sound with:
<pre><code> 
  $ aplay <(./toneGenerator -e "123456")
</code></pre>