#include "Timer2.h"

unsigned long Timer2::time_units;
void (*Timer2::func)();
volatile unsigned long Timer2::count;
volatile char Timer2::overflowing;
volatile unsigned int Timer2::tcnt2;

void Timer2::set(unsigned long ms, void (*f)()) {
    Timer2::set(ms, 0.001, f);
}


/**
 * @param resolution
 *   0.001 implies a 1 ms (1/1000s = 0.001s = 1ms) resolution. Therefore,
 *   0.0005 implies a 0.5 ms (1/2000s) resolution. And so on.
 */
void Timer2::set(unsigned long units, double resolution, void (*f)()) {
	float prescaler = 0.0;
	
	if (units == 0)
		time_units = 1;
	else
		time_units = units;
		
	func = f;
	
	TIMSK2 &= ~(1<<TOIE2);
	TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
	TCCR2B &= ~(1<<WGM22);
	ASSR &= ~(1<<AS2);
	TIMSK2 &= ~(1<<OCIE2A);
	
	if ((F_CPU >= 1000000UL) && (F_CPU <= 16000000UL)) {	// prescaler set to 64
		TCCR2B |= (1<<CS22);
		TCCR2B &= ~((1<<CS21) | (1<<CS20));
		prescaler = 64.0;
	} else if (F_CPU < 1000000UL) {	// prescaler set to 8
		TCCR2B |= (1<<CS21);
		TCCR2B &= ~((1<<CS22) | (1<<CS20));
		prescaler = 8.0;
	} else { // F_CPU > 16Mhz, prescaler set to 128
		TCCR2B |= ((1<<CS22) | (1<<CS20));
		TCCR2B &= ~(1<<CS21);
		prescaler = 128.0;
	}
	
	tcnt2 = 256 - (int)((float)F_CPU * resolution / prescaler);
}

void Timer2::start() {
	count = 0;
	overflowing = 0;
	TCNT2 = tcnt2;
	TIMSK2 |= (1<<TOIE2);
}

void Timer2::stop() {
	TIMSK2 &= ~(1<<TOIE2);
}

void Timer2::_overflow() {
	count += 1;
	
	if (count >= time_units && !overflowing) {
		overflowing = 1;
		count = count - time_units; // subtract time_uints to catch missed overflows
					// set to 0 if you don't want this.
		(*func)();
		overflowing = 0;
	}
}

ISR(TIMER2_OVF_vect) {
	TCNT2 = Timer2::tcnt2;
	Timer2::_overflow();
}
