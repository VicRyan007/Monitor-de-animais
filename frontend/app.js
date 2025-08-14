// --- Configurações do Cliente MQTT ---
const brokerIp = '192.168.3.2'; // O MESMO IP DO SEU BROKER
const brokerPort = 9001; // Porta para WebSockets (configurar no mosquitto)
const topic = 'esp32/dht11/data';
const clientId = 'webapp_client_' + Math.random().toString(16).substr(2, 8);

// --- Elementos da Página ---
const statusConexaoEl = document.getElementById('status-conexao');
const temperaturaEl = document.getElementById('temperatura-valor');
const umidadeEl = document.getElementById('umidade-valor');
const sensacaoEl = document.getElementById('sensacao-valor');
const statusGeralEl = document.getElementById('status-geral');
const statusConfortoEl = document.getElementById('status-conforto');
const timestampEl = document.getElementById('timestamp');
const deviceIdEl = document.getElementById('device-id');

// --- Lógica MQTT ---
const client = new Paho.MQTT.Client(brokerIp, brokerPort, clientId);

client.onConnectionLost = onConnectionLost;
client.onMessageArrived = onMessageArrived;

// Objeto de opções de conexão CORRIGIDO (sem a propriedade 'reconnect')
const options = {
    onSuccess: onConnect,
    onFailure: onFailure,
};

function connect() {
    console.log(`Conectando ao broker MQTT em ws://${brokerIp}:${brokerPort}...`);
    client.connect(options);
}

function onConnect() {
    console.log("Conectado ao broker com sucesso!");
    statusConexaoEl.textContent = 'Conectado';
    statusConexaoEl.className = 'status-conectado';
    client.subscribe(topic);
    console.log(`Inscrito no tópico: ${topic}`);
}

function onFailure(response) {
    console.error("Falha na conexão: " + response.errorMessage);
    statusConexaoEl.textContent = 'Falha na Conexão';
    statusConexaoEl.className = 'status-desconectado';
}

function onConnectionLost(responseObject) {
    if (responseObject.errorCode !== 0) {
        console.log("Conexão perdida: " + responseObject.errorMessage);
        statusConexaoEl.textContent = 'Conexão Perdida';
        statusConexaoEl.className = 'status-desconectado';
    }
}

function onMessageArrived(message) {
    console.log("Mensagem recebida: " + message.payloadString);
    try {
        const data = JSON.parse(message.payloadString);
        
        temperaturaEl.textContent = data.temperatura.toFixed(1) + ' °C';
        umidadeEl.textContent = data.umidade.toFixed(1) + ' %';
        sensacaoEl.textContent = data.indice_calor.toFixed(1) + ' °C';
        
        statusConfortoEl.textContent = data.status_conforto;
        deviceIdEl.textContent = data.device_id;
        
        // --- LINHA CORRIGIDA ---
        timestampEl.textContent = new Date().toLocaleString('pt-BR');
        
        // Atualiza a cor do card de status
        statusGeralEl.className = 'status-card'; // Limpa classes antigas
        statusGeralEl.classList.add('status-' + data.status_conforto.toLowerCase());

    } catch (e) {
        console.error("Erro ao processar a mensagem JSON:", e);
    }
}

// Inicia a conexão
connect();