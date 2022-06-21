# gen_slide_pot_table.py

import math

# FIRST: Fiddle with the actual readings to get Target_readings.

Readings_low = (3, 75, 497, 941, 1018)  # slider at (0, 1/4, 1/2, 3/4, 1)
#print("Readings_low", Readings_low)

Readings_high = tuple(1023 - n for n in Readings_low)   # (1020, 948, 526, 82, 5)
#print("Readings_high", Readings_high)

Half_reading = 1023/2

Low_adjustment = Half_reading / Readings_low[2]
Adj_readings_low = tuple(n * Low_adjustment for n in Readings_low[:3])
#print("Adj_readings_low", Adj_readings_low)

High_adjustment = Half_reading / Readings_high[2]
Adj_readings_high = tuple(n * High_adjustment for n in Readings_high[2:])
#print("Adj_readings_high", Adj_readings_high)

# Average the 1/4 Adj_readings_low and Adj_readings_high
Target_quarter_reading = (Adj_readings_low[1] + Adj_readings_high[1]) / 2
#print("Target_quarter_reading", Target_quarter_reading)


# First scaled 1/4 reading (75) by reading for 1/2 (497) vs. 511.5 giving 77.188
# Next, scaled (1024 - 3/4 reading) (83) by 
# Target readings: got these by averaging the 1/2 to 1/4 and 3/4 to 1/2 readings
Target_readings = [0, Target_quarter_reading, Half_reading]
Target_readings += [1023 - n for n in reversed(Target_readings[:-1])]
#print("Target_readings", Target_readings)

# SECOND: Fit exponential curve K*X**P + C == R,
#         where P is pot position (0-1), and R is reading
#
# Thus:
#
#  K*X**(1/2) + C == Half_reading
#  K*X**(1/4) + C == Target_quarter_reading
#  K*X**0 + C == 0
#
# So, from the last equation, C == -K, substituting for C:
#
#  K*(X**(1/2) - 1) == Half_reading
#  K*(X**(1/4) - 1) == Target_quarter_reading
#
# Solving for X:
#
#  K*(X**(1/2) - 1)         Half_reading
#  ----------------  == ----------------------
#  K*(X**(1/4) - 1)     Target_quarter_reading
#
#  X**(1/2) - 1 == Half_reading / Target_quarter_reading * (X**(1/4) - 1)
#
#  letting x be X**(1/4), and ratio be Half_reading / Target_quarter_reading:
#
#  x**2 == ratio * X - ratio + 1
#
# Using the quadratric equation:




# Quadratic parameters: (x here is X**(1/4))
a = 1
b = -Half_reading / Target_quarter_reading
c = -b - 1
#print("a", a, "b", b, "c", c)

# for the subtract sqrt case, this becomes:
#
#      -b +/- sqrt(b**2 + 4(b + 1))
#  x = ----------------------------
#                  2
#
#      -b +/- sqrt(b**2 + 4b + 4)
#  x = --------------------------
#                  2
#
#      -b +/- sqrt((b + 2)**2)
#  x = -----------------------
#                  2
#
#      -b +/- (b + 2)
#  x = --------------
#            2
#
# So x == 1, or x == -b - 1 == c.  x == 1 makes no sense, so x == c

x = c
#print("x", x)

X = x**4
#print("X", X)

K = Target_readings[2] / (x**2 - 1)
#print("K", K)

# Now, solving K*X**P - K == R for P:
#
#   K*(X**P - 1) == R
#
#   X**P == R/K + 1
#
#   P == log(R/K + 1, X) == log(R/K + 1) / log(X)

log_X = math.log(X)
#print("log_X", log_X)


def conv_low(reading):  # reading is 0 - 511
    # Converts low reading to position (0 - 0.5)
    return math.log(reading/K + 1) / log_X

def conv_high(reading): # reading is 512 - 1023
    # Converts high reading to position (0.5 - 1)
    return 1 - math.log((1023 - reading)/K + 1) / log_X

print()
print("// Indexed by low reading from analogRead() (0 - 511).")
print("// Gives linear position of slider (0 - 127)")
print("const byte Slide_pot_translation[] = {")
first = 0
print("  ", end='')
for i in range(512):
    print(f"{round(conv_low(i) * 127):3}, ", end='');
    if (i + 1) % 10 == 0:
        print(f"    // {first:3} - {i:3}")
        first = i + 1
        print("  ", end='')
print(' ' * (8 * 5), "    // 510 - 511", sep='')
"""

first = i + 1
print('  ', ' ' * (2 * 5), sep='', end='')
for i in range(512, 1024):
    print(f"{round(conv_high(i) * 127):3}, ", end='');
    if (i + 1) % 10 == 0:
        print(f"    // {first:3} - {i:3}")
        first = i + 1
        print("  ", end='')
print(' ' * (6 * 5), f"    // {first:3} - {i:3}", sep='')
"""
print("};")
print()

print("""
byte scale_slide_pot(int reading, int calibrated_low, int calibrated_center,
                     int calibrated_high
) {
  // `reading` is the direct output of analogRead on the slide pot (0 - 1023).
  // `calibrated_low` and `calibrated_high` are the distances from each endpoint of the
  // slide pot to consider the same as the endpoint.  E.g., calibrated_low of 3 would
  // treat 3 as 0, and calibrated_high of 1020 would threat 1020 as 1023.
  // `calibrated_center` is the reading at the detented center point on the slide pot.
  //
  // Returns the scaled (linearized) value between 0 and 127.
  //
  // FIX: Should 0.5 be added to the subscripts (to round them)?  Or should they simply be
  //      truncated?
  int numerator, denominator;
  if (reading <= calibrated_center) {
    numerator = max(0, reading - calibrated_low);
    denominator = calibrated_center - calibrated_low;
    return Slide_pot_translation[(numerator * 511L + denominator / 2) / denominator];
  }
  numerator = max(0, calibrated_high - reading);
  denominator = calibrated_high - (calibrated_center + 1);
  return 127 - Slide_pot_translation[(numerator * 511L + denominator / 2) / denominator];
}
""")
