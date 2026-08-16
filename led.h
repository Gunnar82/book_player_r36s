#ifndef LED_H
#define LED_H

void led_init(void);
void led_set(int on);
int led_get_gpio(void);
int led_gpio_is_manual(void);
int led_set_gpio_manual(int gpio);
int led_reset_gpio_auto(void);
int led_gpio_available(int gpio);

#endif /* LED_H */
