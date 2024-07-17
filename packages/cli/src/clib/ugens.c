#ifndef UGENS_H
#define UGENS_H

#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE 44100
#define MAX_DELAY_TIME 10
#define DELAY_BUFFER_LENGTH (MAX_DELAY_TIME * SAMPLE_RATE)
#define SAMPLE_TIME (1.0 / SAMPLE_RATE)
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define RANDOM_FLOAT ((float)arc4random() / (float)UINT32_MAX)

// helpers

double lerp(double x, double y0, double y1)
{
  if (x >= 1)
    return y1;
  return y0 + x * (y1 - y0);
}

// SineOsc

typedef struct SineOsc
{
  double phase;
} SineOsc;

void SineOsc_init(SineOsc *osc)
{
  osc->phase = 0.0;
}

double SineOsc_update(SineOsc *osc, double freq, double x, double y)
{
  osc->phase += SAMPLE_TIME * freq;
  if (osc->phase >= 1.0)
    osc->phase -= 1.0; // Keeping phase in [0, 1)
  return sin(osc->phase * 2.0 * M_PI);
}

void *SineOsc_create()
{
  void *osc = malloc(sizeof(SineOsc));
  SineOsc_init((SineOsc *)osc);
  return (void *)osc;
}

// SawOsc

typedef struct SawOsc
{
  double phase;
} SawOsc;

void SawOsc_init(SawOsc *osc)
{
  osc->phase = 0.0;
}

/* double phasor_update(double phase, double freq)
{
  phase += SAMPLE_TIME * freq;
  if (phase >= 1.0)
    phase -= 1.0; // Keeping phase in [0, 1)
  return phase;
} */

double SawOsc_update(SawOsc *osc, double freq)
{
  osc->phase += SAMPLE_TIME * freq;
  if (osc->phase >= 1.0)
    osc->phase -= 1.0; // Keeping phase in [0, 1)
  return (osc->phase) * 2 - 1;
}

void *SawOsc_create()
{
  void *osc = malloc(sizeof(SawOsc));
  SawOsc_init((SawOsc *)osc);
  return (void *)osc;
}

// ADSRNode

// ADSRNode structure and state enumeration
typedef enum
{
  ADSR_OFF,
  ADSR_ATTACK,
  ADSR_DECAY,
  ADSR_SUSTAIN,
  ADSR_RELEASE
} ADSRState;

typedef struct ADSRNode
{
  ADSRState state;
  double startTime;
  double startVal;
} ADSRNode;

void ADSRNode_init(ADSRNode *env)
{
  env->state = ADSR_OFF;
  env->startTime = 0.0;
  env->startVal = 0.0;
}

double ADSRNode_update(ADSRNode *env, double curTime, double gate, double attack, double decay, double susVal, double release)
{
  switch (env->state)
  {
  case ADSR_OFF:
    if (gate > 0)
    {
      env->state = ADSR_ATTACK;
      env->startTime = curTime;
      env->startVal = 0.0;
    }
    return 0.0;

  case ADSR_ATTACK:
  {
    double time = curTime - env->startTime;
    if (time > attack)
    {
      env->state = ADSR_DECAY;
      env->startTime = curTime;
      return 1.0;
    }
    return lerp(time / attack, env->startVal, 1.0);
  }

  case ADSR_DECAY:
  {
    double time = curTime - env->startTime;
    double curVal = lerp(time / decay, 1.0, susVal);

    if (gate <= 0)
    {
      env->state = ADSR_RELEASE;
      env->startTime = curTime;
      env->startVal = curVal;
      return curVal;
    }

    if (time > decay)
    {
      env->state = ADSR_SUSTAIN;
      env->startTime = curTime;
      return susVal;
    }

    return curVal;
  }

  case ADSR_SUSTAIN:
    if (gate <= 0)
    {
      env->state = ADSR_RELEASE;
      env->startTime = curTime;
      env->startVal = susVal;
    }
    return susVal;

  case ADSR_RELEASE:
  {
    double time = curTime - env->startTime;
    if (time > release)
    {
      env->state = ADSR_OFF;
      return 0.0;
    }

    double curVal = lerp(time / release, env->startVal, 0.0);
    if (gate > 0)
    {
      env->state = ADSR_ATTACK;
      env->startTime = curTime;
      env->startVal = curVal;
    }
    return curVal;
  }
  }

  // Fallback case for invalid state
  // fprintf(stderr, "Invalid ADSR state\n");
  return 0.0;
}

void *ADSRNode_create()
{
  ADSRNode *env = (ADSRNode *)malloc(sizeof(ADSRNode));
  ADSRNode_init(env);
  return (void *)env;
}

// Filter

typedef struct Filter
{
  double s0;
  double s1;
} Filter;

void Filter_init(Filter *self)
{
  self->s0 = 0;
  self->s1 = 0;
}

double Filter_update(Filter *self, double input, double cutoff, double resonance)
{

  // Out of bound values can produce NaNs
  cutoff = fmin(cutoff, 1);
  resonance = fmax(resonance, 0);

  double c = pow(0.5, (1 - cutoff) / 0.125);
  double r = pow(0.5, (resonance + 0.125) / 0.125);
  double mrc = 1 - r * c;

  double v0 = self->s0;
  double v1 = self->s1;

  // Apply the filter to the sample
  v0 = mrc * v0 - c * v1 + c * input;
  v1 = mrc * v1 + c * v0;
  double output = v1;

  self->s0 = v0;
  self->s1 = v1;

  return output;
}

void *Filter_create()
{
  Filter *env = (Filter *)malloc(sizeof(Filter));
  Filter_init(env);
  return (void *)env;
}

// ImpulseOsc

typedef struct ImpulseOsc
{
  double phase;
} ImpulseOsc;

void ImpulseOsc_init(ImpulseOsc *self)
{
  self->phase = 1;
}

double ImpulseOsc_update(ImpulseOsc *self, double freq, double phase)
{
  self->phase += SAMPLE_TIME * freq;
  double v = self->phase >= 1 ? 1 : 0;
  if (self->phase >= 1.0)
    self->phase -= 1.0; // Keeping phase in [0, 1)
  return v;
}

void *ImpulseOsc_create()
{
  ImpulseOsc *env = (ImpulseOsc *)malloc(sizeof(ImpulseOsc));
  ImpulseOsc_init(env);
  return (void *)env;
}

// Lag

int lagUnit = 4410;

typedef struct Lag
{
  double s;
} Lag;

void Lag_init(Lag *self)
{
  self->s = 0;
}

double Lag_update(Lag *self, double input, double rate)
{
  // Remap so the useful range is around [0, 1]
  rate = rate * lagUnit;
  if (rate < 1)
    rate = 1;
  self->s += (1 / rate) * (input - self->s);
  return self->s;
}

void *Lag_create()
{
  Lag *env = (Lag *)malloc(sizeof(Lag));
  Lag_init(env);
  return (void *)env;
}

// Delay

typedef struct Delay
{
  int writeIdx;
  int readIdx;
  float buffer[DELAY_BUFFER_LENGTH];
} Delay;

void Delay_init(Delay *self)
{
  // Write and read positions in the buffer
  self->writeIdx = 0;
  self->readIdx = 0;
}

double Delay_update(Delay *self, double input, double time)
{

  self->writeIdx = (self->writeIdx + 1) % DELAY_BUFFER_LENGTH;
  self->buffer[self->writeIdx] = input;

  // Calculate how far in the past to read
  int numSamples = MIN(
      floor(SAMPLE_RATE * time),
      DELAY_BUFFER_LENGTH - 1);

  self->readIdx = self->writeIdx - numSamples;

  // If past the start of the buffer, wrap around
  if (self->readIdx < 0)
    self->readIdx += DELAY_BUFFER_LENGTH;

  return self->buffer[self->readIdx];
}

void *Delay_create()
{
  Delay *node = (Delay *)malloc(sizeof(Delay));
  Delay_init(node);
  return (void *)node;
}

typedef struct Feedback
{
  double value;
} Feedback;

void Feedback_init(Feedback *self)
{
  self->value = 0;
}

double Feedback_write(Feedback *self, double value)
{
  self->value = value;
  return 0;
}
double Feedback_update(Feedback *self)
{
  return self->value;
}

void *Feedback_create()
{
  Feedback *node = (Feedback *)malloc(sizeof(Feedback));
  Feedback_init(node);
  return (void *)node;
}

// Fold

typedef struct Fold
{
} Fold;

void Fold_init(Fold *self)
{
}

double Fold_update(Fold *self, double input, double rate)
{
  // Make it so rate 0 means input unaltered because
  // NoiseCraft knobs default to the [0, 1] range
  if (rate < 0)
    rate = 0;
  rate = rate + 1;

  input = input * rate;
  return (
      4 *
      (fabs(0.25 * input + 0.25 - roundf(0.25 * input + 0.25)) - 0.25));
}

void *Fold_create()
{
  Fold *node = (Fold *)malloc(sizeof(Fold));
  Fold_init(node);
  return (void *)node;
}

// BrownNoiseOsc

typedef struct BrownNoiseOsc
{
  float out;
} BrownNoiseOsc;

void BrownNoiseOsc_init(BrownNoiseOsc *self)
{
  self->out = 0;
}

double BrownNoiseOsc_update(BrownNoiseOsc *self)
{
  float white = RANDOM_FLOAT * 2.0 - 1.0;
  self->out = (self->out + 0.02 * white) / 1.02;
  return self->out;
}

void *BrownNoiseOsc_create()
{
  BrownNoiseOsc *node = (BrownNoiseOsc *)malloc(sizeof(BrownNoiseOsc));
  BrownNoiseOsc_init(node);
  return (void *)node;
}

// PinkNoise

typedef struct PinkNoise
{
  float b0;
  float b1;
  float b2;
  float b3;
  float b4;
  float b5;
  float b6;
} PinkNoise;

void PinkNoise_init(PinkNoise *self)
{

  self->b0 = 0;
  self->b1 = 0;
  self->b2 = 0;
  self->b3 = 0;
  self->b4 = 0;
  self->b5 = 0;
  self->b6 = 0;
}

double PinkNoise_update(PinkNoise *self)
{

  float white = RANDOM_FLOAT * 2 - 1;

  self->b0 = 0.99886 * self->b0 + white * 0.0555179;
  self->b1 = 0.99332 * self->b1 + white * 0.0750759;
  self->b2 = 0.969 * self->b2 + white * 0.153852;
  self->b3 = 0.8665 * self->b3 + white * 0.3104856;
  self->b4 = 0.55 * self->b4 + white * 0.5329522;
  self->b5 = -0.7616 * self->b5 - white * 0.016898;

  float pink =
      self->b0 +
      self->b1 +
      self->b2 +
      self->b3 +
      self->b4 +
      self->b5 +
      self->b6 +
      white * 0.5362;
  self->b6 = white * 0.115926;

  return pink * 0.11;
}

void *PinkNoise_create()
{
  PinkNoise *node = (PinkNoise *)malloc(sizeof(PinkNoise));
  PinkNoise_init(node);
  return (void *)node;
}

// NoiseOsc

typedef struct NoiseOsc
{
} NoiseOsc;

void NoiseOsc_init(NoiseOsc *self)
{
}

double NoiseOsc_update(NoiseOsc *self)
{
  return RANDOM_FLOAT * 2 - 1;
}

void *NoiseOsc_create()
{
  NoiseOsc *node = (NoiseOsc *)malloc(sizeof(NoiseOsc));
  NoiseOsc_init(node);
  return (void *)node;
}

#endif // UGENS_H

/*

- [x] ADSRNode
- [-] AudioIn
- [x] BrownNoiseOsc
- [-] CC
- [-] Clock
- [ ] ClockDiv
- [-] ClockOut
- [x] Delay
- [ ] Distort
- [ ] DustOsc
- [x] Feedback
- [x] Filter
- [x] Fold
- [ ] Hold
- [x] ImpulseOsc
- [x] Lag
- [-] MidiCC
- [-] MidiFreq
- [-] MidiGate
- [-] MidiIn
- [x] NoiseOsc
- [x] PinkNoise
- [ ] PulseOsc
- [x] SawOsc
- [ ] Sequence
- [x] SineOsc
- [ ] Slew
- [ ] Slide
- [ ] TriOsc

*/

/*

// Template

typedef struct Template
{
} Template;

void Template_init(Template *self)
{
}

double Template_update(Template *self)
{
}

void *Template_create()
{
  Template *node = (Template *)malloc(sizeof(Template));
  Template_init(node);
  return (void *)node;
}

*/