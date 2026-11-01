extern int control_input, control_output;
extern volatile int control_device;
extern void control_call (int);
extern int control_tail (int);

int Prefer (void)
{
  int value;
  control_call (0);
  if (control_input)
    {
      control_output = 23;
      value = 31;
    }
  else
    value = 47;
  return control_tail (value);
}

int Live (int value)
{
  control_call (0);
  if (control_input)
    {
      control_output = value;
      value += 7;
    }
  else
    value = 47;
  return control_tail (value);
}

int PreferOffset (int seed)
{
  int value;
  control_call (0);
  if (control_input)
    {
      control_output = 23;
      value = seed + 31;
    }
  else
    value = seed + 47;
  return control_tail (value);
}

int Trap (int *pointer)
{
  int value;
  control_call (0);
  if (control_input)
    {
      control_output = 23;
      value = 31;
    }
  else
    value = *pointer;
  return control_tail (value);
}

int Volatile (void)
{
  int value;
  control_call (0);
  if (control_input)
    {
      control_output = 23;
      value = 31;
    }
  else
    value = control_device;
  return control_tail (value);
}

int Shared (int other)
{
  control_call (0);
  if (other)
    goto shared;
  if (control_input)
    {
      control_output = 23;
      return control_tail (31);
    }
shared:
  return control_tail (47);
}

int Strong (void)
{
  int value = 0;
  while (control_input)
    {
      control_call (value);
      value += 7;
    }
  return value;
}

int StrongNE (void)
{
  int value;
  control_call (0);
  if (!control_input)
    {
      control_output = 23;
      value = 31;
    }
  else
    value = 47;
  return control_tail (value);
}
