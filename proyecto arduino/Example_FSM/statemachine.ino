/*!
\file   statemachine.ino
\date   2024-30-11
\author Glenn Ward, Jose Rodríguez, Camilo Benavidez
\brief  Máquina de estados

\par Copyright
La información contenida en este documento es propiedad y constituye valiosos 
secretos comerciales confidenciales de Unicauca, y está sujeta a restricciones 
de uso y divulgación.

\par
Copyright (c) Unicauca 2024. Todos los derechos reservados.

\par
Los avisos de derechos de autor anteriores no evidencian ninguna publicación 
real o prevista de este material.
*******************************************************************************/

/*!
\brief Configura las transiciones y acciones de la máquina de estados.
*/
void setup_State_Machine() {
  // Transiciones desde el estado Inicio
  stateMachine.AddTransition(Inicio, Monitoreo_ambiental, []() {
    return currentInput == clavecorrecta;
  });
  stateMachine.AddTransition(Inicio, State_bloqueado, []() {
    return currentInput == bloqueado;
  });
  stateMachine.AddTransition(State_bloqueado, Inicio, []() {
    return currentInput == Sign_T;
  });

  // Transiciones entre Monitoreo_ambiental y Monitor_eventos
  stateMachine.AddTransition(Monitoreo_ambiental, Monitor_eventos, []() {
    return currentInput == Sign_T2;
  });
  stateMachine.AddTransition(Monitor_eventos, Monitoreo_ambiental, []() {
    return currentInput == Sign_T;
  });

  // Transiciones a los estados de alerta y alarma
  stateMachine.AddTransition(Monitoreo_ambiental, Alarma_roja, []() {
    return currentInput == Sign_D;
  });
  stateMachine.AddTransition(Monitor_eventos, Alerta_azul, []() {
    return currentInput == Sign_D;
  });
  stateMachine.AddTransition(Alerta_azul, Monitor_eventos, []() {
    return currentInput == Sign_T3;
  });
  stateMachine.AddTransition(Alerta_azul, Alarma_roja, []() {
    return currentInput == Sign_P;
  });
  stateMachine.AddTransition(Alarma_roja, Inicio, []() {
    return currentInput == Sign_A;
  });

  // Acciones al entrar a los estados
  stateMachine.SetOnEntering(Inicio, input_Inicio);
  stateMachine.SetOnEntering(Monitoreo_ambiental, input_Monitoreo_ambiental);
  stateMachine.SetOnEntering(Monitor_eventos, input_Monitor_eventos);
  stateMachine.SetOnEntering(State_bloqueado, input_State_bloqueado);
  stateMachine.SetOnEntering(Alerta_azul, input_Alerta_azul);
  stateMachine.SetOnEntering(Alarma_roja, input_Alarma_roja);

  // Acciones al salir de los estados
  stateMachine.SetOnLeaving(Inicio, output_Inicio);
  stateMachine.SetOnLeaving(Monitoreo_ambiental, output_Monitoreo_ambiental);
  stateMachine.SetOnLeaving(State_bloqueado, output_State_bloqueado);
  stateMachine.SetOnLeaving(Monitor_eventos, output_Monitor_eventos);
  stateMachine.SetOnLeaving(Alerta_azul, output_Alerta_azul);
  stateMachine.SetOnLeaving(Alarma_roja, output_Alarma_roja);
}

/*!
\brief Maneja las acciones al entrar en el estado Inicio.
*/
void input_Inicio() {
  lcd.clear();
  lcd.print("Digite la contraseña");
  contrasenia.Start();
}

/*!
\brief Maneja las acciones al salir del estado Inicio.
*/
void output_Inicio() {
  digitalWrite(LED_GREEN_PIN, LOW);
  contrasenia.Stop();
  lcd.clear();
}

/*!
\brief Inicia la medición de variables ambientales al entrar en Monitoreo_ambiental.
*/
void input_Monitoreo_ambiental() {
  Temperatura_Ambiente.Start();
  tiempo_espera.Start();
}

/*!
\brief Detiene la medición de variables al salir de Monitoreo_ambiental.
*/
void output_Monitoreo_ambiental() {
  Temperatura_Ambiente.Stop();
  tiempo_espera.Stop();
  lcd.clear();
}

/*!
\brief Configura el estado bloqueado al entrar en State_bloqueado.
*/
void input_State_bloqueado() {
  lcd.print("Sistema bloqueado");
  tarea_bloqueado.Start();
  tiempo_espera_bloqueado.Start();
}

/*!
\brief Restaura las condiciones iniciales al salir de State_bloqueado.
*/
void output_State_bloqueado() {
  tarea_bloqueado.Stop();
  tiempo_espera_bloqueado.Stop();
}

/*!
\brief Configura la visualización y los sensores en Monitor_eventos.
*/
void input_Monitor_eventos() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IR: ");
  lcd.print("OFF");
  lcd.setCursor(0, 1);
  lcd.print("SH:");
  lcd.print("OFF");
  tarea_sensores.Start();
  tiempo_espera_eventos.Start();
}

/*!
\brief Detiene los sensores y limpia la interfaz al salir de Monitor_eventos.
*/
void output_Monitor_eventos() {
  tarea_sensores.Stop();
  tiempo_espera_eventos.Stop();
}

/*!
\brief Activa la alerta azul al entrar en Alerta_azul.
*/
void input_Alerta_azul() {
  lcd.clear();
  lcd.print("ALERTA");
  tiempo_espera_alerta_azul.Start();
  tarea_buzzer_azul.Start();
}

/*!
\brief Detiene la alerta azul al salir de Alerta_azul.
*/
void output_Alerta_azul() {
  tarea_buzzer_azul.Stop();
}

/*!
\brief Activa la alarma roja y permite reiniciar el sistema al entrar en Alarma_roja.
*/
void input_Alarma_roja() {
  lcd.clear();
  lcd.print("ALARMA");
  tarea_reseteo.Start();
  tarea_buzzer_rojo.Start();
}

/*!
\brief Detiene la alarma roja y resetea el sistema al salir de Alarma_roja.
*/
void output_Alarma_roja() {
  lcd.clear();
  lcd.print("Reiniciando");
  tarea_reseteo.Stop();
  tarea_buzzer_rojo.Stop();
}
