# Comments on Development of mmspa Library

This is a pure C library. ANY C++ feature is not welcome. So far, it is tightly
coupled with PostgreSQL/PostGIS database, which is not good. The external data
source will become configurable in the future.

## Coding Standards

### Public functions 

#### Internal public functions

The functions are visible to all across the library, but invisible to external
clients.

```C
void DoSomething();
```

#### External public functions

The functions are visible to all across the library as well as the external
clients. In other words, they are the APIs. Here I use the MSP instead of MMSPA for 
abbrevating Multimodal Shortest Paths because it is more concise.

```C
void MSPdoSomething();
```

### Constants

```C
ALL_CAPITALS_WITH_UNDERSCORES_SEPERATED
```

### Local functions, variables and parameters

All private functions should be declared as `static`. The first letter should
be lowercase.

```C
static int localVar;
static void localFunction(int firstParam, char *secondParam);
```

### Member variables

```C
struct Demo {
    int all_lowercase_with_underscores;
}
```

### Curly braces

```C
for (i = 0; i < 10; i++) {
    doSomething();
    doSomeOtherThing();
}
```

```C
if (cond > 0) {
    doThis();
    doThat();
} else {
    doSomething();
    doSomethingElse();
}
```
