/***********************************************************************************

   Simple stack oriented pocket calculator with touch screen input.

   Bernd Ulmann, 12.07.2021

 **********************************************************************************/

#include <TouchScreen.h>
#include <LCDWIKI_GUI.h>
#include <LCDWIKI_KBV.h>

#ifndef TRUE
#define TRUE 1
#define FALSE !TRUE
#endif

LCDWIKI_KBV disp(ILI9486, A3, A2, A1, A0, A4); //model,cs,cd,wr,rd,reset

#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define STACK_SIZE    128       // Size of stack
#define STO_SIZE      128       // Number of storage registers
#define STRING_LENGTH 40
#define STACK_Y_START 40        // Start y coordinate for stack display
#define STACK_LINES   4         // Number of lines for stack display
#define STACK_LINE_Y  20        // Height of a single stack display line

#define INPUT_Y       160       // Y position of input field

#define KBD_X      4            // Number of keys in a column
#define KBD_Y      8            // Number of keys in a row
#define KBD_DX     4            // x distance between two adjacent keys
#define KBD_DY     2            // y distance between two adjacent keys
#define KEY_HEIGHT 30           // Height of one key

#define KEY_TOUCH_DX  100       // Delta x for touch screen per key with respect to middle of key
#define KEY_TOUCH_DY  30        // As above but delta y.
#define MIN_PRESSURE  5         // Minimum pressure to activate touch screen
#define KEY_RELEASE_DELAY 500   // Delay for key release test

#define DEFAULT_PRECISION 7     // Number of decimal digits to be displayed

#define E  2.71828182845904

#define STATE_IDLE     0        // States for the state machine for number input
#define STATE_INT      1        // No decimal point has been entered so far
#define STATE_FRACTION 2        // Number input is behind the decimal point

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);  // The TFT display used here has 300 Ohms of resistance accross the X axis.

String keys[] = {"0", ".", "+-", "PUSH",
                 "1", "2", "3", "+ -",
                 "4", "5", "6", "* /",
                 "7", "8", "9", "1/X",
                 "STO", "X/Y", "LOG", "LN",
                 "SIN", "COS", "TAN", "P/R",
                 "PI/E", "I/FP", "SQRT", "DEL",
                 "RAD", "FIX", "CLR", "INV"
                };

bool rad = TRUE,    // Radians if TRUE, otherwise degrees
     inv  = FALSE,  // Inverse flag, true if INV was pressed in the preceeding step
     error = FALSE; // Global error flag

uint16_t fix = DEFAULT_PRECISION, // Number of decimal digits in fix format
         state = STATE_IDLE,      // State for the number input state machine
         sp = 0;                  // Stack pointer - points to the next free stack location

float stack[STACK_SIZE],
      storage[STO_SIZE];

char number[STRING_LENGTH] = "+";

void display_string(String str, int16_t x, int16_t y, uint8_t text_size, uint16_t color, boolean mode)
{
  disp.Set_Text_Mode(mode);
  disp.Set_Text_Size(text_size);
  disp.Set_Text_colour(color);
  disp.Print_String(str, x, y);
}

void display_status(String key) {
  char mode[STRING_LENGTH];

  disp.Set_Draw_color(BLACK);
  disp.Fill_Rectangle(0, 0, disp.Get_Display_Width(), 10);
  sprintf(mode, "% 4s SP=%03d F=%02d %s", rad ? "RAD  " : "DEG  ", sp, fix, inv ? "I" : " ");
  display_string(mode, 0, 0, 1, YELLOW, 1);

  if (error)
    display_string("ERROR", disp.Get_Display_Width() - 80, 0, 1, RED, 1);

  display_string(key, disp.Get_Display_Width() - 40, 0, 1, RED, 1);
}

void display_stack() {  // Display a maximum of STACK_LINES of stack content
  char line[STRING_LENGTH];

  disp.Set_Draw_color(BLACK);
  disp.Fill_Rectangle(0, STACK_Y_START, disp.Get_Display_Width(), STACK_LINES * STACK_LINE_Y + STACK_Y_START);

  uint16_t y = 0;
  for (int16_t i = min(sp, STACK_LINES); i > 0; i--)
    display_string(String(stack[sp - i], fix), 0, y++ * STACK_LINE_Y + STACK_Y_START, 2, WHITE, 1);
}

void clear_machine() {
  for (uint16_t i = 0; i < STACK_SIZE; stack[i++] = 0.);
  for (uint16_t i = 0; i < STO_SIZE; storage[i++] = 0.);
  sp = 0;
  inv = FALSE;
  rad = TRUE;
  fix = DEFAULT_PRECISION;
}

void draw_keys(String keys[]) {
  uint16_t key_width = disp.Get_Display_Width() / KBD_X,
           display_height = disp.Get_Display_Height();

  for (uint16_t y = 0; y < KBD_Y; y++) {
    for (uint16_t x = 0; x < KBD_X; x++) {
      disp.Set_Draw_color(WHITE);
      disp.Fill_Round_Rectangle(x * key_width + KBD_DX, display_height - (y * KEY_HEIGHT + KBD_DY), (x + 1) * key_width - KBD_DX, display_height - ((y + 1) * KEY_HEIGHT - KBD_DY), 5);
      display_string(keys[y * KBD_X + x], x * key_width + (key_width >> 2), display_height - (y * KEY_HEIGHT + (KEY_HEIGHT / 1.5) + 2), 2, BLUE, 1);
    }
  }
}

void push(float value) {
  if (sp < STACK_SIZE) {
    stack[sp++] = value;
    //    display_stack();
  } else
    error = TRUE;
}

float pop() {      // This function does not update the stack display as this causes excessive flicker
  if (sp == 0) {
    error = TRUE;
    return 0.;
  }

  return stack[--sp];
}

boolean read_touch_screen(int16_t *x, int16_t *y) {
  static uint16_t col[KBD_X] = {190, 390, 590, 790},
                               row[KBD_Y] = {110, 165, 220, 272, 327, 380, 434, 490};
  TSPoint p;

  digitalWrite(13, HIGH);
  p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  if (p.z > MIN_PRESSURE) {
    *x = *y = -1;
    for (uint16_t i = 0; i < KBD_X; i++)
      if (abs(p.x - col[i]) < KEY_TOUCH_DX)
        *x = i;

    for (uint16_t i = 0; i < KBD_Y; i++)
      if (abs(p.y - row[i]) < KEY_TOUCH_DY)
        *y = i;
    /*
        if (*x >= 0 && *y >= 0) { // Wait for key release
          do {
            delay(KEY_RELEASE_DELAY);
            digitalWrite(13, HIGH);
            p = ts.getPoint();
            digitalWrite(13, LOW);
            pinMode(XM, OUTPUT);
            pinMode(YP, OUTPUT);
          } while (p.z <= MIN_PRESSURE);
          return TRUE;
        }
    */
    return *x >= 0 && *y >= 0;
  }

  return FALSE;
}

void number_input(String key) {

  if (state == STATE_IDLE) {  // We start with a new number
    strcpy(number, "+");
    state = STATE_INT;        // and the digits entered make up the integer part of the number
  }

  if (state == STATE_INT || state == STATE_FRACTION) {
    if (key == "0")      strcat(number, "0");
    else if (key == "1") strcat(number, "1");
    else if (key == "2") strcat(number, "2");
    else if (key == "3") strcat(number, "3");
    else if (key == "4") strcat(number, "4");
    else if (key == "5") strcat(number, "5");
    else if (key == "6") strcat(number, "6");
    else if (key == "7") strcat(number, "7");
    else if (key == "8") strcat(number, "8");
    else if (key == "9") strcat(number, "9");
    else if (key == "+-" && !inv) { // Change sign
      display_string("+", 0, INPUT_Y, 2, BLACK, 1);
      if (number[0] != '+')
        number[0] = '+';
      else
        number[0] = '-';
    }
    else if (key == "." && state == STATE_INT) {
      strcat(number, ".");
      state == STATE_FRACTION;
    }

    display_string(number, 0, INPUT_Y, 2, GREEN, 1);
  }
}

void clear_number_input() {
  display_string(number, 0, INPUT_Y, 2, BLACK, 1);
}

void loop()
{
  int16_t key_x, key_y;
  String key_pressed;
  float tos = 0., tos_1 = 0.;

  delay(KEY_RELEASE_DELAY);

  if (read_touch_screen(&key_x, &key_y)) {  // Key pressed
    error = FALSE;
    key_pressed = keys[key_x + key_y * KBD_X];

    //*********************************** Row 0 ***********************************
    if (key_pressed == "RAD") {
      if (inv)
        rad = inv = FALSE;
      else
        rad = TRUE;
    }
    else if (key_pressed == "FIX") {
      tos = pop();
      if (!error)
        fix = min(abs(tos), DEFAULT_PRECISION);
    }
    else if (key_pressed == "CLR") {
      clear_machine();
    }
    else if (key_pressed == "INV") {
      inv = !inv;
    }

    //*********************************** Row 1 ***********************************
    else if (key_pressed == "PI/E") {
      if (inv) {
        push(E);
        inv = FALSE;
      } else
        push(PI);
    }
    else if (key_pressed == "I/FP") {
      tos = pop();
      if (!error) {
        if (inv) {
          push(tos - (int) tos);
          inv = FALSE;
        } else
          push((int) tos);
      }
    }
    else if (key_pressed == "SQRT") {
      tos = pop();
      if (!error) {
        if (inv) {  // square
          push(tos * tos);
          inv = FALSE;
        } else
          push(sqrt(tos));
      }
    }
    else if (key_pressed == "DEL") {
      clear_number_input();
      strcpy(number, "+");
      state = STATE_IDLE;
    }

    //*********************************** Row 2 ***********************************
    else if (key_pressed == "SIN") {
      tos = pop();
      if (!error) {
        if (inv) {  // asin(...)
          if (rad)
            push(asin(tos));
          else
            push(asin(tos) * 180. / PI);
          inv = FALSE;
        } else {
          if (rad)
            push(sin(tos));
          else  // Degrees
            push(sin(tos / 180. * PI));
        }
      }
    }
    else if (key_pressed == "COS") {
      tos = pop();
      if (!error) {
        if (inv) {  // acos()
          if (rad)
            push(acos(tos));
          else
            push(acos(tos) * 180. / PI);
          inv = FALSE;
        } else {
          if (rad)
            push(cos(tos));
          else
            push(cos(tos / 180. * PI));
        }
      }
    }
    // TODO: Take care of overflows
    else if (key_pressed == "TAN") {
      tos = pop();
      if (!error) {
        if (inv) {  // atan(...)
          if (rad)
            push(atan(tos));
          else
            push(atan(tos) * 180. / PI);
          inv = FALSE;
        } else {
          if (rad)
            push(tan(tos));
          else
            push(tan(tos / 180. * PI));
        }
      }
    }
    else if (key_pressed == "P/R") {
      if (sp < 2)
        error = TRUE;
      else {
        tos = pop();
        tos_1 = pop();

        if (inv) {  // Rectangular to polar
          inv = FALSE;
          float r = sqrt(tos * tos + tos_1 * tos_1);
          push(r);
          if (rad)
            push(asin(tos_1 / r));
          else
            push(asin(tos_1 / r) * 180. / PI);
        } else {    // Polar to rectangular
          if (rad) {
            push(tos_1 * cos(tos));
            push(tos_1 * sin(tos));
          } else {
            push(tos_1 * cos(tos / 180. * PI));
            push(tos_1 * sin(tos / 180. * PI));
          }
        }
      }
    }

    //*********************************** Row 3 ***********************************
    else if (key_pressed == "STO") {
      if (inv) {  // Recall
        inv = FALSE;
        tos = pop();
        if (!error)
          push(storage[(int) tos]);
      } else {
        if (sp < 2)
          error = TRUE;
        else {
          tos = pop();
          tos_1 = pop();
  
          storage[(int) tos % STO_SIZE] = tos_1;
        }      
      }
    }
    else if (key_pressed == "X/Y") {
      if (sp < 2)
        error = TRUE;
      else {
        tos = pop();
        tos_1 = pop();

        push(tos);
        push(tos_1);
      }
    }
    else if (key_pressed == "LOG") {
      tos = pop();
      if (!error) {
        if (inv) {  // 10 ** ...
          push(exp(tos * log(10)));
          inv = FALSE;
        } else      // log(...)
          push(log(tos) / log(10));
      }
    }
    else if (key_pressed == "LN") {
      tos = pop();
      if (!error) {
        if (inv) {  // exp(...)
          push(exp(tos));
          inv = FALSE;
        } else      // ln(...)
          push(log(tos));
      }
    }

    //*********************************** Row 4 ***********************************
    else if (key_pressed == "1/X") {
      tos = pop();
      if (!error) {
        if (tos == 0.)
          error = TRUE;
        else
          push(1 / tos);
        state == STATE_IDLE;
      }
    }

    //*********************************** Row 5 ***********************************
    else if (key_pressed == "* /") {
      if (sp < 2)
        error = TRUE;
      else {
        tos = pop();
        tos_1 = pop();

        if (inv) {
          push(tos_1 / tos);
          inv = FALSE;
        } else
          push(tos_1 * tos);
      }
    }

    //*********************************** Row 6 ***********************************
    else if (key_pressed == "+ -") {
      if (sp < 2)
        error = TRUE;
      else {
        tos = pop();
        tos_1 = pop();

        if (inv) {
          push(tos_1 - tos);
          inv = FALSE;
        } else
          push(tos_1 + tos);
      }
    }

    //*********************************** Row 7 ***********************************
    else if (key_pressed == "PUSH") {
      if (inv) {
        pop();
        inv = FALSE;
      } else {
        if (state == STATE_IDLE) {  // Duplicate the number on TOS
          tos = pop();
          if (!error) {
            push(tos);
            push(tos);
          }
        } else {
          String value = number;
          clear_number_input();
          push(value.toDouble());
          state = STATE_IDLE;
        }
      }
    }

    //*********************************** Digits, decimal point, +- ***********************************
    else if (key_pressed == "0" || key_pressed == "1" || key_pressed == "2" || key_pressed == "3" ||
             key_pressed == "4" || key_pressed == "5" || key_pressed == "6" || key_pressed == "7" ||
             key_pressed == "8" || key_pressed == "9" || key_pressed == "." || key_pressed == "+-" ||
             key_pressed == "DEL")
      number_input(key_pressed);

    display_status(key_pressed);
    display_stack();
  }
}

void setup(void)
{
  disp.Init_LCD();
  disp.Fill_Screen(BLACK);
  draw_keys(keys);
  display_status("    ");
  clear_machine();
  display_stack();
}
