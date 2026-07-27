#ifndef RELAY_H
#define RELAY_H

/* ============================== PUBLIC API ============================== */

// Initialize relay GPIO (must be called in setup)
void Relay_Init();
// Turn relay ON (activate output)
void Relay_On();
// Turn relay OFF (deactivate output)
void Relay_Off();

#endif