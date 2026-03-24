from fastapi import FastAPI, Request
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates

app = FastAPI()
templates = Jinja2Templates(directory="templates")

# Estado em memória — persiste enquanto o servidor estiver rodando
curtidas = 0

@app.get("/", response_class=HTMLResponse)
async def index(request: Request):
    return templates.TemplateResponse("index.html", {"request": request})

# Exercício 1 e 2: recebe POST de curtir e zerar
@app.post("/curtir", response_class=HTMLResponse)
async def curtir(request: Request):
    global curtidas
    curtidas += 1
    return templates.TemplateResponse("curtidas.html", {"request": request, "curtidas": curtidas})

@app.post("/zerar", response_class=HTMLResponse)
async def zerar(request: Request):
    global curtidas
    curtidas = 0
    return templates.TemplateResponse("curtidas.html", {"request": request, "curtidas": curtidas})

# Exercício 3: rotas para carregar cada aba
@app.get("/aba/curtidas", response_class=HTMLResponse)
async def aba_curtidas(request: Request):
    return templates.TemplateResponse("curtidas.html", {"request": request, "curtidas": curtidas})

@app.get("/aba/jupiter", response_class=HTMLResponse)
async def aba_jupiter(request: Request):
    return templates.TemplateResponse("jupiter.html", {"request": request})

@app.get("/aba/professor", response_class=HTMLResponse)
async def aba_professor(request: Request):
    return templates.TemplateResponse("professor.html", {"request": request})