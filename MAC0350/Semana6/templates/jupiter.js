// jupiter.js

function showToast(msg, duration = 2500) {
    const toast = document.getElementById('toast');
    toast.textContent = msg;
    toast.classList.add('show');
    setTimeout(() => toast.classList.remove('show'), duration);
}

// Cliques nos botões de menu
function menuClick(nome) {
    showToast(`Navegando para: ${nome}`);
}

// Submit do login
function handleLogin(event) {
    event.preventDefault();
    const usuario = event.target.usuario.value.trim();
    const senha   = event.target.senha.value.trim();

    if (!usuario || !senha) {
        showToast('Preencha usuário e senha.');
        return;
    }

    // Aqui futuramente: fetch POST /login
    showToast(`Entrando como ${usuario}…`);
}