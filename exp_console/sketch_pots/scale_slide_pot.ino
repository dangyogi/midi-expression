
// Indexed by low reading from analogRead() (0 - 511).
// Gives linear position of slider (0 - 127)
const byte Slide_pot_translation[] = {
    0,   1,   2,   3,   4,   5,   6,   6,   7,   8,     //   0 -   9
    8,   9,  10,  10,  11,  12,  12,  13,  13,  14,     //  10 -  19
   14,  15,  15,  16,  16,  17,  17,  17,  18,  18,     //  20 -  29
   19,  19,  19,  20,  20,  21,  21,  21,  22,  22,     //  30 -  39
   22,  23,  23,  23,  23,  24,  24,  24,  25,  25,     //  40 -  49
   25,  25,  26,  26,  26,  27,  27,  27,  27,  28,     //  50 -  59
   28,  28,  28,  28,  29,  29,  29,  29,  30,  30,     //  60 -  69
   30,  30,  30,  31,  31,  31,  31,  31,  32,  32,     //  70 -  79
   32,  32,  32,  33,  33,  33,  33,  33,  34,  34,     //  80 -  89
   34,  34,  34,  34,  35,  35,  35,  35,  35,  35,     //  90 -  99
   36,  36,  36,  36,  36,  36,  36,  37,  37,  37,     // 100 - 109
   37,  37,  37,  37,  38,  38,  38,  38,  38,  38,     // 110 - 119
   38,  39,  39,  39,  39,  39,  39,  39,  39,  40,     // 120 - 129
   40,  40,  40,  40,  40,  40,  40,  41,  41,  41,     // 130 - 139
   41,  41,  41,  41,  41,  42,  42,  42,  42,  42,     // 140 - 149
   42,  42,  42,  42,  43,  43,  43,  43,  43,  43,     // 150 - 159
   43,  43,  43,  44,  44,  44,  44,  44,  44,  44,     // 160 - 169
   44,  44,  44,  45,  45,  45,  45,  45,  45,  45,     // 170 - 179
   45,  45,  45,  45,  46,  46,  46,  46,  46,  46,     // 180 - 189
   46,  46,  46,  46,  46,  47,  47,  47,  47,  47,     // 190 - 199
   47,  47,  47,  47,  47,  47,  47,  48,  48,  48,     // 200 - 209
   48,  48,  48,  48,  48,  48,  48,  48,  48,  49,     // 210 - 219
   49,  49,  49,  49,  49,  49,  49,  49,  49,  49,     // 220 - 229
   49,  49,  50,  50,  50,  50,  50,  50,  50,  50,     // 230 - 239
   50,  50,  50,  50,  50,  50,  51,  51,  51,  51,     // 240 - 249
   51,  51,  51,  51,  51,  51,  51,  51,  51,  51,     // 250 - 259
   52,  52,  52,  52,  52,  52,  52,  52,  52,  52,     // 260 - 269
   52,  52,  52,  52,  52,  52,  53,  53,  53,  53,     // 270 - 279
   53,  53,  53,  53,  53,  53,  53,  53,  53,  53,     // 280 - 289
   53,  53,  54,  54,  54,  54,  54,  54,  54,  54,     // 290 - 299
   54,  54,  54,  54,  54,  54,  54,  54,  54,  55,     // 300 - 309
   55,  55,  55,  55,  55,  55,  55,  55,  55,  55,     // 310 - 319
   55,  55,  55,  55,  55,  55,  55,  56,  56,  56,     // 320 - 329
   56,  56,  56,  56,  56,  56,  56,  56,  56,  56,     // 330 - 339
   56,  56,  56,  56,  56,  56,  57,  57,  57,  57,     // 340 - 349
   57,  57,  57,  57,  57,  57,  57,  57,  57,  57,     // 350 - 359
   57,  57,  57,  57,  57,  57,  58,  58,  58,  58,     // 360 - 369
   58,  58,  58,  58,  58,  58,  58,  58,  58,  58,     // 370 - 379
   58,  58,  58,  58,  58,  58,  58,  59,  59,  59,     // 380 - 389
   59,  59,  59,  59,  59,  59,  59,  59,  59,  59,     // 390 - 399
   59,  59,  59,  59,  59,  59,  59,  59,  59,  59,     // 400 - 409
   60,  60,  60,  60,  60,  60,  60,  60,  60,  60,     // 410 - 419
   60,  60,  60,  60,  60,  60,  60,  60,  60,  60,     // 420 - 429
   60,  60,  60,  61,  61,  61,  61,  61,  61,  61,     // 430 - 439
   61,  61,  61,  61,  61,  61,  61,  61,  61,  61,     // 440 - 449
   61,  61,  61,  61,  61,  61,  61,  61,  62,  62,     // 450 - 459
   62,  62,  62,  62,  62,  62,  62,  62,  62,  62,     // 460 - 469
   62,  62,  62,  62,  62,  62,  62,  62,  62,  62,     // 470 - 479
   62,  62,  62,  62,  63,  63,  63,  63,  63,  63,     // 480 - 489
   63,  63,  63,  63,  63,  63,  63,  63,  63,  63,     // 490 - 499
   63,  63,  63,  63,  63,  63,  63,  63,  63,  63,     // 500 - 509
   63,  63,                                             // 510 - 511
};


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

