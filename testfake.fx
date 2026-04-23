# flux_bruteforce_demo.fx
# A self-contained brute-force demo written entirely in Flux

write "=============================================="
write "    Flux Offline Brute-Force Demo"
write "=============================================="
wait for user
     write options ["1. Numbers (1-999)"; "2. Letters (a-z)"]
     say "Enter 1 or 2"

# ------------------------------------------------------------
# TEMPLATE: Number brute-force
# ------------------------------------------------------------
\-\
     write "[*] Starting numeric brute-force..."
     from numbers: 1 to 999 step 1
     use functl.combination till output message says
          from output "Access granted"
     write "[+] Done. If no success message, target not cracked."
\-\
function add + ("NUM_BRUTE")

# ------------------------------------------------------------
# TEMPLATE: Letter brute-force (single lowercase letter)
# ------------------------------------------------------------
\-\
     write "[*] Starting alphabetic brute-force..."
     from letters: a to z
     use functl.combination till output message says
          from output "Access granted"
     write "[+] Done."
\-\
function add + ("ALPHA_BRUTE")

# ------------------------------------------------------------
# Main logic: dispatch based on user input
# ------------------------------------------------------------
if user input is "1" use
     codestring "NUM_BRUTE"

if user input is "2" use
     codestring "ALPHA_BRUTE"

if user input is not "1" and user input is not "2"
     write "Invalid choice. Exiting."
