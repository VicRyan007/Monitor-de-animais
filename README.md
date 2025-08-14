# Sistema de Monitoramento de Temperatura e Umidade para Bem-Estar Animal

Este repositório contém o código-fonte completo para um sistema de monitoramento IoT, desenvolvido como parte da disciplina de Sistemas Embarcados.

## Descrição do Projeto

O bem-estar de animais domésticos ou de criação depende diretamente das condições do ambiente em que vivem. Temperaturas excessivamente altas ou baixas podem causar estresse, problemas de saúde e até levar à morte. Este projeto resolve a falta de um método acessível e automatizado para monitorar essas condições, desenvolvendo um sistema de baixo custo que informa em tempo real a temperatura e a umidade de um local, permitindo que cuidadores tomem ações preventivas para garantir a segurança e o conforto dos animais.

A solução foi implementada utilizando um microcontrolador ESP32 como cérebro do sistema, conectado a um sensor DHT11 para a coleta dos dados. O firmware, desenvolvido em C com o framework ESP-IDF, não apenas realiza a leitura dos dados brutos, mas também calcula informações enriquecidas, como o índice de calor (sensação térmica) e um status de conforto (ex: "Confortável", "Quente", "Perigoso").

A comunicação é realizada através do protocolo MQTT. O ESP32 atua como um cliente "publisher", conectando-se a uma rede Wi-Fi local e enviando as informações em formato JSON para um broker Mosquitto. Para a visualização dos dados, foi desenvolvido um frontend web (HTML, CSS e JavaScript) que atua como um cliente "subscriber", conectando-se ao broker via WebSockets e exibindo os dados em um dashboard dinâmico e de fácil leitura, que se atualiza em tempo real.

## Como Executar o Projeto

Para replicar e executar este projeto, você precisará das seguintes ferramentas e de seguir os passos abaixo.

### Ferramentas Necessárias

1.  **ESP-IDF v5.4.1:** O framework de desenvolvimento para o ESP32.
2.  **Mosquitto MQTT Broker:** O intermediário para as mensagens MQTT.
3.  **Python 3:** Para rodar um servidor web local para o frontend.
4.  **Git:** Para clonar este repositório.

---

### Passo 1: Configurar o Broker MQTT (Mosquitto)

O Mosquitto é o "carteiro" que recebe as mensagens do ESP32 e as entrega para a página web.

**Download do Mosquitto:**
* Acesse a página oficial de downloads do Mosquitto: [https://mosquitto.org/download/](https://mosquitto.org/download/)
* Na seção "Windows", baixe o instalador apropriado para a sua versão do Windows (geralmente 64-bit).

**Instalação:**
* Execute o instalador que você baixou.
* Siga as instruções na tela. É recomendado manter o caminho de instalação padrão (`C:\Program Files\mosquitto`). Durante a instalação, ele instalará o broker como um serviço do Windows, o que significa que ele iniciará automaticamente com o computador.

**Configuração para WebSockets:**
* Por padrão, o Mosquitto só aceita conexões MQTT normais. Precisamos habilitar o suporte a WebSockets para que o nosso frontend (a página web) consiga se conectar.
* Navegue até a pasta de instalação: `C:\Program Files\mosquitto`.
* Abra o arquivo `mosquitto.conf` com um editor de texto (como o Bloco de Notas, executado como administrador).
* Vá até o final do arquivo e adicione as seguintes linhas:
    ```conf
    # Listener padrão para o ESP32 e outros dispositivos
    listener 1883

    # Listener para o frontend web (WebSockets)
    listener 9001
    protocol websockets

    # Permite conexões de clientes sem usuário/senha
    allow_anonymous true
    ```

**Reinicie o Serviço:**
* Para que as novas configurações tenham efeito, o serviço do Mosquitto precisa ser reiniciado.
* Pressione `Win` + `R` para abrir a caixa "Executar".
* Digite `services.msc` e pressione Enter.
* Na lista, procure por "Mosquitto Broker".
* Clique com o botão direito sobre ele e selecione "Reiniciar".

Após esses passos, seu broker MQTT estará pronto para receber conexões tanto do ESP32 quanto da sua página web.

---

### Passo 2: Configurar o Backend (Sistema Embarcado)

O backend é o firmware que roda no ESP32.

1.  **Clone o Repositório:**
    ```bash
    git clone [https://github.com/VicRyan007/Monitor-de-animais.git](https://github.com/VicRyan007/Monitor-de-animais.git)
    cd Monitor-de-animais
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
    *Para descobrir o IP do seu computador, abra o CMD e digite `ipconfig`. Use o "Endereço IPv4".*

4.  **Compile e Grave:**
    Abra um terminal com o ambiente ESP-IDF ativado, conecte o ESP32 ao computador e execute o comando abaixo, substituindo `SUA_PORTA` pela porta COM correta (ex: `COM8`):
    ```bash
    idf.py -p SUA_PORTA flash monitor
    ```
    O terminal deverá mostrar os logs de conexão e, em seguida, as leituras do sensor.

---

### Passo 3: Executar o Frontend (Dashboard Web)

O frontend é uma página web que visualiza os dados.

1.  **Abra um Novo Terminal:**
    Não use o mesmo terminal do ESP32.

2.  **Navegue até a Pasta `frontend`:**
    Dentro da pasta do projeto que você clonou, entre na pasta `frontend`.
    ```bash
    cd frontend
    ```

3.  **Inicie o Servidor Web:**
    Execute o comando do Python para criar um servidor local:
    ```bash
    python -m http.server
    ```

4.  **Acesse o Dashboard:**
    Abra seu navegador de internet e acesse o seguinte endereço: [**http://localhost:8000**](http://localhost:8000)

Se todos os passos foram seguidos corretamente, o dashboard mostrará o status "Conectado" e começará a exibir os dados de temperatura e umidade enviados pelo seu ESP32.
