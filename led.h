#ifndef LED_H
#define LED_H

void led_init(void);
void led_set(int on);
int led_get_gpio(void);
int led_gpio_is_manual(void);
int led_set_gpio_manual(int gpio);
int led_reset_gpio_auto(void);
int led_gpio_available(int gpio);

/* LED-Test: Im Testmodus werden normale LED-Anforderungen ignoriert.
   led_test_tick() wechselt den Zustand alle 500 ms. */
void led_set_test_mode(int enabled);
int led_test_mode(void);
void led_test_tick(unsigned int now_ms);

#endif /* LED_H */
