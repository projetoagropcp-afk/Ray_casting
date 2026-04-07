# Ray_casting
Este código implementa um sistema de Geofencing (cerca virtual) utilizando um microcontrolador ESP32 e o módulo GNSS de alta precisão Unicore Communications UM980. Ele utiliza o algoritmo de Ray Casting para determinar se o dispositivo está dentro ou fora de um perímetro pré-definido.


GNSS Telemetry & Geofencing (Ray Casting) System
Este projeto consiste em um sistema de telemetria GNSS desenvolvido para ESP32, focado em monitoramento de localização em tempo real e segurança de perímetro. O sistema processa dados de satélite, registra logs em um cartão SD e fornece feedback visual imediato através de LEDs.

Principais Funcionalidades
Geofencing por Ray Casting: Implementação do algoritmo matemático de Ray Casting para verificar se uma coordenada (latitude/longitude) está contida dentro de um polígono complexo.

Telemetria de Alta Precisão: Integração com o módulo Unicore UM980 via biblioteca SparkFun, permitindo o uso de RTK para precisão centimétrica.

Data Logging Dual: Gravação de dados em tempo real no formato CSV em um cartão SD, separando logs gerais de navegação e alertas específicos de violação de perímetro.

Feedback Visual Não-Bloqueante: * LED de Status GPS: Pisca enquanto busca sinal e permanece aceso quando a posição é fixada.

LED de Alerta: Pisca rapidamente quando o dispositivo cruza o limite para fora do perímetro definido.

Arquitetura do Código
Hardware Interface: Configura comunicação UART para o GNSS e SPI para o módulo de cartão SD.

Algoritmo de Perímetro: A função pontoDentroDoPoligono percorre os vértices do polígono poligonoPerimetro[] e inverte o estado booleano de "dentro" cada vez que um raio imaginário cruza uma aresta do polígono.

Loop de Controle: Executa a cada 1000ms para processamento de dados e logging, enquanto mantém os processos de piscar LEDs de forma independente através da função millis(), garantindo que o sistema não trave (zero delay()).

Hardware Utilizado
Microcontrolador: ESP32

Módulo GNSS: Unicore Communications UM980 (ou compatíveis da série UM900)

Armazenamento: Módulo MicroSD Card

Indicadores: LEDs (Onboard e Externos)

Estrutura de Saída (CSV)
Os logs gravados no SD e na Serial seguem o formato:
Timestamp_MS, Latitude, Longitude, Altitude_m, SIV (Satélites), PosType, DentroArea
