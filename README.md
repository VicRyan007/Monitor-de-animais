# Sistema de Monitoramento de Temperatura e Umidade para Bem-Estar Animal

Este repositório contém o código-fonte completo para um sistema de monitoramento IoT, desenvolvido como parte da disciplina de Sistemas Embarcados.

## Descrição do Projeto

O bem-estar de animais domésticos ou de criação depende diretamente das condições do ambiente em que vivem. Temperaturas excessivamente altas ou baixas podem causar estresse, problemas de saúde e até levar à morte. Este projeto resolve a falta de um método acessível e automatizado para monitorar essas condições, desenvolvendo um sistema de baixo custo que informa em tempo real a temperatura e a umidade de um local, permitindo que cuidadores tomem ações preventivas para garantir a segurança e o conforto dos animais.

A solução foi implementada utilizando um microcontrolador ESP32 como cérebro do sistema, conectado a um sensor DHT11 para a coleta dos dados. O firmware, desenvolvido em C++ com o framework ESP-IDF, não apenas realiza a leitura dos dados brutos, mas também calcula informações enriquecidas, como o índice de calor (sensação térmica) e um status de conforto (ex: "Confortável", "Quente", "Perigoso").

A comunicação é realizada através do protocolo MQTT. O ESP32 atua como um cliente "publisher", conectando-se a uma rede Wi-Fi local e enviando as informações em formato JSON para um broker Mosquitto. Para a visualização dos dados, foi desenvolvido um frontend web (HTML, CSS e JavaScript) que atua como um cliente "subscriber", conectando-se ao broker via WebSockets e exibindo os dados em um dashboard dinâmico e de fácil leitura, que se atualiza em tempo real.


## Como Executar o Projeto

Para replicar e executar este projeto, você precisará das seguintes ferramentas e de seguir os passos abaixo.

### Ferramentas Necessárias

1.  **ESP-IDF v5.4.1:** O framework de desenvolvimento para o ESP32.
2.  **Mosquitto MQTT Broker:** O intermediário para as mensagens MQTT.
3.  **Python 3:** Para rodar um servidor web local para o frontend.
4.  **Git:** Para clonar este repositório.

---

### Passo 1: Configurar o Backend (Sistema Embarcado)

O backend é o firmware que roda no ESP32.

1.  **Clone o Repositório:**
    ```bash
    git clone <URL_DO_SEU_REPOSITORIO>
    cd <NOME_DO_REPOSITORIO>
    ```

2.  **Conecte o Hardware:**
    Conecte o sensor DHT11 ao ESP32 conforme a tabela abaixo:

| Pino no Módulo DHT11 | Pino no ESP32 |
| :--- | :--- |
| **`+`** (Energia) | **`3V3`** |
| **`out`** (Dados) | **`D4`** |
| **`-`** (Terra) | **`GND`** |

3.  **Configure as Credenciais:**
    Abra o arquivo `main/main.c` e edite as seguintes linhas com as informações da sua rede local:
    ```c
    #define WIFI_SSID      "NOME_DA_SUA_REDE_WIFI"
    #define WIFI_PASS      "SENHA_DA_SUA_REDE_WIFI"
    #define MQTT_BROKER_IP "IP_DO_COMPUTADOR_COM_MOSQUITTO"
    ```

4.  **Compile e Grave:**
    Abra um terminal com o ambiente ESP-IDF ativado, conecte o ESP32 ao computador e execute o comando abaixo, substituindo `SUA_PORTA` pela porta COM correta (ex: `COM8`):
    ```bash
    idf.py -p SUA_PORTA flash monitor
    ```
    O terminal deverá mostrar os logs de conexão e, em seguida, as leituras do sensor.

---

### Passo 2: Configurar o Broker MQTT

O Mosquitto precisa ser configurado para aceitar conexões do ESP32 e do frontend web.

1.  **Edite o Arquivo de Configuração:**
    Abra o arquivo `mosquitto.conf` (geralmente em `C:\Program Files\mosquitto`).

2.  **Adicione as Linhas:**
    No final do arquivo, adicione as seguintes linhas para habilitar a porta padrão e a porta para WebSockets:
    ```conf
    listener 1883
    listener 9001
    protocol websockets
    allow_anonymous true
    ```

3.  **Inicie o Mosquitto:**
    A forma mais fácil é rodá-lo como um serviço do Windows. Abra `services.msc`, encontre "Mosquitto Broker" e clique em **"Iniciar"** ou **"Reiniciar"**.

---

### Passo 3: Executar o Frontend (Dashboard Web)

O frontend é uma página web que visualiza os dados.

1.  **Abra um Novo Terminal:**
    Não use o mesmo terminal do ESP32.

2.  **Navegue até a Pasta `frontend`:**
    ```bash
    cd frontend
    ```

3.  **Inicie o Servidor Web:**
    Execute o comando do Python para criar um servidor local:
    ```bash
    python -m http.server
    ```

4.  **Acesse o Dashboard:**
    Abra seu navegador de internet e acesse o seguinte endereço:
    [**http://localhost:8000**](http://localhost:8000)

Se todos os passos foram seguidos corretamente, o dashboard mostrará o status "Conectado" e começará a exibir os dados de temperatura e umidade enviados pelo seu ESP32.