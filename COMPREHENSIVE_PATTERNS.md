# Comprehensive Pattern Coverage Needed

## Current (36 patterns) - TOO LIMITED!

## What's Missing:

### 1. Sparse Character Sets (Shufti)
- [aeiou]*, [aeiou]+, [aeiouAEIOU]*
- [02468]*, [13579]*
- [!?.,:;]* (punctuation)

### 2. Mixed Ranges
- [a-zA-Z]*, [a-zA-Z]+
- [0-9a-fA-F]* (hex)
- [a-zA-Z0-9_]* (identifier)

### 3. Negated Classes
- [^a-z]*, [^0-9]*
- [^\s]*, [^\n]*

### 4. Larger Inputs
- 256, 512, 1024 byte tests

### 5. Real-World Patterns
- Email-like: [a-zA-Z0-9._%-]+@[a-zA-Z0-9.-]+
- URL-like: https?://[a-zA-Z0-9.-]+
- IPv4: [0-9]{1,3}\.[0-9]{1,3}...

### 6. Case-Insensitive
- (?i)[a-z]+, (?i)hello

### 7. Whitespace
- \s*, \s+, [ \t\n]+

**TARGET: 80-100 patterns for proper coverage**
