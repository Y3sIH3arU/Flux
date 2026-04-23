write "=== Flux Cybersecurity Toolkit ==="
write "1. Password Bruteforcer"
write "2. Wordlist Generator"
wait for user

if user said "1" then reply "Starting bruteforce..."

define bruteforce_numbers as
    write "Trying: 1"
    write "Trying: 2"
    write "Trying: 3"
    write "Access granted!"
end

define bruteforce_letters as
    write "Trying: a"
    write "Trying: b"
    write "Trying: c"
    write "Access granted!"
end

if user said "1" then bruteforce_numbers
if user said "2" then bruteforce_letters

\-
    write "Template codestring body"
    write "Inside the template"
\-
function add + ("TESTCODE")

use codestring "TESTCODE"