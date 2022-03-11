int add5(int i)
{
  return i+5;
}

int add10(int i)
{
  i = add5(i);
  return add5(i);
}