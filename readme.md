# HyDrata - Monitoramento Hídrico e Irrigação Inteligente (Módulo IoT)

Esta documentação detalha o módulo de **Internet das Coisas (IoT)** da plataforma **HyDrata**, desenvolvido para a entrega da **Global Solution 2026/1** da FIAP para a disciplina **Disruptive Architectures: IoT, IoB & Generative IA**.

---

## 👥 Integrantes do Grupo
*   **Ana Flavia Camelo** - RM561489
*   **Gustavo Kenji Terada** - RM562745
*   **João Guilherme Carvalho Novaes** - RM566234
*   **Pedro Chasci Puga** - RM565154
*   **Lucas Figueiredo Vieira** - RM561342
*   **Turma**: 2TDSPV

---

## 📖 Visão Geral do Projeto HyDrata

O **HyDrata** é uma plataforma de monitoramento hídrico e otimização de irrigação desenhada para ajudar pequenos e médios produtores rurais a tomarem decisões inteligentes de manejo, além de alertá-los contra desastres ecológicos (secas severas, enchentes locais e focos de incêndio próximos). 

### 🚨 O Problema
Pequenos e médios agricultores costumam irrigar suas lavouras de forma intuitiva, gerando desperdício desnecessário de água e energia elétrica (acionamento excessivo de bombas). Ao mesmo tempo, dados técnicos cruciais gerados por satélites e órgãos oficiais (como ANA, INPE e INMET) não chegam de forma acessível e direta a quem está no campo. A falta de informações consolidadas resulta em decisões tomadas às cegas e alta vulnerabilidade a intempéries climáticas.

### 💡 A Solução Principal
O HyDrata centraliza dados locais medidos por sensores (umidade, temperatura e luminosidade) e os cruza com fontes de dados espaciais (satélites meteorológicos e de monitoramento). A principal entrega para o produtor é uma recomendação simplificada: 
> **"IRRIGAR HOJE? SIM ou NÃO"**

### 📈 Benefícios Mensuráveis
*   **Economia de Recurso**: Redução de até 40% no consumo de água nas lavouras.
*   **Eficiência Energética**: Menor desgaste e economia na conta de luz associada às bombas d'água.
*   **Prevenção de Riscos**: Alertas rápidos sobre enchentes em rios monitorados na região e focos de incêndio ativos em um raio de 30 km.
*   **Produtividade**: Regas feitas no momento biológico exato de necessidade da planta.

---

## 🧱 Arquitetura Geral da Solução (Integração de Stacks)

O ecossistema HyDrata funciona através da integração de 7 camadas de tecnologia, nas quais o módulo IoT atua como captador e atuador na ponta física (Edge).

```mermaid
graph TD
    A["1. Satélites (ANA, INPE, NASA, INMET)"] -->|APIs Rest| C["3. Backend APIs (Java / .NET)"]
    B["2. Dispositivo IoT (ESP32)"] -->|Protocolo MQTT (TLS)| C
    C -->|Persistência| D["4. Banco de Dados (Oracle 19c)"]
    C -->|Diagnósticos/Prompting| G["IA Generativa (Geração de Mensagens)"]
    C -->|Deploy em Containers| E["5. DevOps & Cloud (Docker / Azure)"]
    C -->|Consumo REST| F["6. App Mobile (React Native)"]
    H["7. Compliance & Governança (TOGAF / ArchiMate)"] -.- C
```

### Detalhamento das Camadas e Stacks:

1.  **Fontes Espaciais**: Consumo das APIs públicas do **INPE** (BDQueimadas), **NASA** (FIRMS), **ANA** (HidroWebService) e **Open-Meteo** para obter previsões de chuvas e monitoramento de satélite.
2.  **Módulo IoT (ESP32)**: O circuito físico monitora o microclima local em tempo real e envia os dados consolidados via rede Wi-Fi para a nuvem usando o protocolo **MQTT**.
3.  **Backend APIs**:
    *   **Java (Spring Boot)**: Processa as regras de negócio centrais, consome as APIs espaciais e orquestra a geração de alertas integrados.
    *   **C# (.NET)**: Responsável pelo gerenciamento de cadastros de produtores, propriedades agrícolas e cooperativas parceiras.
4.  **Banco de Dados**: Servidor **Oracle 19c** contendo o modelo físico relacional de dados (leituras dos sensores, dados externos das APIs de satélite e logs de alertas gerados).
5.  **DevOps & Cloud**: Empacotamento de toda a infraestrutura em containers **Docker** com deploy orquestrado para a nuvem **Microsoft Azure**.
6.  **Mobile App**: Aplicativo desenvolvido em **React Native** para que o produtor visualize o dashboard de sua fazenda de forma simples e intuitiva.
7.  **Compliance**: Documentação formal da arquitetura empresarial do ecossistema segundo a especificação **TOGAF** e modelagem em **ArchiMate**.

---

## 📋 Requisitos da Entrega de IoT & Mapeamento

Abaixo está o mapeamento detalhado de como os requisitos específicos da matéria de IoT estão cobertos pelo protótipo construído:

| Requisito do Edital | Implementação Técnica no HyDrata | Arquivos Referência |
| :--- | :--- | :--- |
| **Microcontrolador ESP32** | Placa de desenvolvimento utilizada como cérebro da ponta física para controle e rede. | [sketch.ino](firmware/sketch.ino) |
| **Mínimo de 2 Entradas** | 1. **Sensor DHT22**: Temperatura e Umidade do Ar.<br>2. **Sensor LDR**: Luminosidade/Irradiação Solar. | [sketch.ino (Linhas 14-16)](firmware/sketch.ino#L14-L16) |
| **Mínimo de 2 Saídas** | 1. **LED Azul (Bomba)**: Simula o acionamento de uma bomba de irrigação.<br>2. **LED Vermelho (Alerta)**: Alerta luminoso para calor térmico extremo. | [sketch.ino (Linhas 17-18)](firmware/sketch.ino#L17-L18) |
| **Interface Local (LCD)** | Display LCD 16x2 (I2C) com carrossel dinâmico não-bloqueante a cada 3 segundos. | [sketch.ino (Linhas 197-230)](firmware/sketch.ino#L197-L230) |
| **Conexão Wi-Fi** | Conexão de rede sem fio automática com suporte a reconexão em caso de queda. | [sketch.ino (Linhas 55-76)](firmware/sketch.ino#L55-L76) |
| **Comunicação MQTT** | Conectividade segura e criptografada (TLS, porta 8883) com o HiveMQ Cloud. | [sketch.ino (Linhas 78-100)](firmware/sketch.ino#L78-L100) |
| **Endpoints JSON/MQTT** | Envio periódico (a cada 5s) de telemetria estruturada em JSON para 3 tópicos distintos. | [sketch.ino (Linhas 174-195)](firmware/sketch.ino#L174-L195) |
| **Dashboard de Dados** | Interface para monitoramento visual dos tópicos MQTT e histórico de leituras. | *[Inserir link do Dashboard]* |
| **Vídeo de até 3 Minutos** | Vídeo contendo a proposta conceitual da solução e o funcionamento do circuito simulado. | *[Inserir link do vídeo]* |

---

## 🛠️ Detalhamento do Circuito IoT

O circuito utiliza componentes clássicos de fácil integração e baixo custo:

*   **DHT22 (Temperatura e Umidade)**: Permite a leitura rápida de temperatura do ar (°C) e umidade relativa (%). Ligado no pino **GPIO 15**.
*   **LDR (Light Dependent Resistor)**: Sensor de luminosidade analógico conectado ao pino **GPIO 34**. O valor cru de tensão de entrada (0 a 4095) é linearmente mapeado para uma escala compreensível de 0% a 100% de radiação solar.
*   **LED Azul**: Indicador de fluxo de irrigação da bomba. Conectado ao pino **GPIO 2**.
*   **LED Vermelho**: Indicador de alerta extremo térmico. Conectado ao pino **GPIO 4**.
*   **Display LCD 16x2 I2C**: Endereço I2C `0x27`, conectado nos pinos padrão do ESP32 (**SDA: GPIO 21** e **SCL: GPIO 22**).

---

## 🧠 Lógica de Controle Local (Edge Computing)

Para garantir o funcionamento mínimo do sistema mesmo na falta temporária de sinal de internet, o ESP32 roda lógica local de controle de atuadores:

1.  **Gatilho de Irrigação (LED Azul)**:
    *   *Regra*: Se a umidade do ar for inferior a **40.0%** **E** a luminosidade ambiente estiver acima de **70%** (indicando dia ensolarado e seco).
    *   *Comportamento*: Liga a bomba d'água para manter a plantação estável. Caso contrário, mantém a bomba desligada para economizar recursos.
2.  **Gatilho de Calor Extremo (LED Vermelho)**:
    *   *Regra*: Se a temperatura ambiente detectada for maior ou igual a **38.0 °C** (risco iminente de desidratação severa ou queimadas próximas).
    *   *Comportamento*: Aciona o alerta crítico.

---

## 📡 Comunicação MQTT e Estrutura JSON

A telemetria é distribuída em formato JSON a cada **5 segundos** por meio dos seguintes tópicos:

### 1. Dados de Clima e Ambiente
*   **Tópico**: `FIAP/HYDRATA/CLIMA`
*   **Payload**:
    ```json
    {
      "temperatura": 28.4,
      "umidade_ar": 39.5
    }
    ```

### 2. Dados de Radiação Solar (Luz)
*   **Tópico**: `FIAP/HYDRATA/LUZ`
*   **Payload**:
    ```json
    {
      "luminosidade": 72
    }
    ```

### 3. Status Operacional dos Atuadores
*   **Tópico**: `FIAP/HYDRATA/STATUS`
*   **Payload**:
    ```json
    {
      "bomba_ativa": true,
      "alerta_critico": false
    }
    ```

## 💻 Como Executar o Projeto de IoT

### Instalação de Bibliotecas no Ambiente Arduino IDE / Wokwi
Você precisará importar as seguintes bibliotecas no seu workspace:
1.  **PubSubClient** (comunicação com o broker MQTT)
2.  **LiquidCrystal I2C** (driver do display LCD)
3.  **DHT sensor library** (driver do sensor de clima)
4.  **Adafruit Unified Sensor** (dependência do driver DHT)

*(Consulte o arquivo [libraries.txt](firmware/libraries.txt) para detalhes sobre as versões).*

### Instruções para Simulação no Wokwi

1.  Abra o [Wokwi](https://wokwi.com).
2.  Crie um circuito utilizando a placa **ESP32 DevKit V1**.
3.  Realize a conexão dos cabos seguindo os pinos configurados no código:
    *   **DHT22**: GPIO `15`
    *   **LDR**: GPIO `34` (analógica)
    *   **LED Azul (Bomba)**: GPIO `2` (LED com resistor em série para o GND)
    *   **LED Vermelho (Alerta)**: GPIO `4` (LED com resistor em série para o GND)
    *   **LCD 16x2 I2C**: SDA na GPIO `21`, SCL na GPIO `22`.
4.  Cole o conteúdo do arquivo [sketch.ino](firmware/sketch.ino) na guia de código principal.
5.  Execute o simulador. O microcontrolador se conectará automaticamente à rede Wi-Fi virtual `Wokwi-GUEST` e enviará a telemetria ao broker do HiveMQ.

### 📊 Painel de Visualização no Node-RED

No repositório, disponibilizamos o arquivo [flows Hydrata.json](flows%20Hydrata.json) pré-configurado para carregar o Dashboard completo do projeto. 

**Como funciona o fluxo do Node-RED:**
*   **Conexão Segura**: O nó `mqtt-broker` se conecta criptografado ao HiveMQ (`2585265c020e418ba116d2601017a480.s1.eu.hivemq.cloud:8883`) com usuário e senha.
*   **Parsing Automático**: Os nós MQTT In (`Sub Clima`, `Sub Luz` e `Sub Status`) estão configurados com o formato `datatype: "json"`, realizando a conversão do payload automaticamente para objeto JavaScript sem a necessidade de nós coletores extras.
*   **Layout Visual**:
    *   **Grupo Microclima Local**: Mostra a temperatura (°C) e a umidade do ar (%) em medidores tipo Gauge.
    *   **Grupo Radiação Solar**: Apresenta a luminosidade mapeada do LDR (0% a 100%).
    *   **Grupo Atuadores & Alertas**: Cartões de texto dinâmicos que exibem o estado formatado em cores e mensagens legíveis da Bomba de Irrigação (LIGADA/DESLIGADA) e do Alerta de Calor Crítico.

**Como Importar o Dashboard:**
1.  Abra a interface do seu **Node-RED**.
2.  Clique no menu superior direito (ícone de hambúrguer com três barras) e vá em **Import**.
3.  Carregue o arquivo [flows Hydrata.json](flows%20Hydrata.json) ou copie todo o seu conteúdo e cole na caixa de texto.
4.  Clique no botão **Import**.
5.  Clique em **Deploy** no canto superior direito do Node-RED.
6.  Acesse a aba visual do Dashboard em `http://localhost:1880/ui` (ou na URL/porta equivalente onde o Node-RED estiver hospedado).

---

## 🔗 Links e entregáveis

*   **Vídeo Youtube (Apresentação)**: *[Inserir link do YouTube ou Drive]*
*   **Código-Fonte Principal (ESP32)**: [sketch.ino](firmware/sketch.ino)
*   **Fluxo do Dashboard (Node-RED)**: [flows Hydrata.json](flows%20Hydrata.json)
*   **Github**: [link do github](https://github.com/vitalis-sa/GLOBAL-SOLUTION-2ANO-1SEM-IOT)

