#ifndef FAKE_GPIO_H
#define FAKE_GPIO_H

void fake_gpio_reset(void);
void fake_gpio_fail_next_writes(int count);
int fake_gpio_write_count(int level);
int fake_gpio_current_level(void);

#endif
