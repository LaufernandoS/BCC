import pandas as pd
from conllu import parse_incr

def ler_conllu(caminho):
    """
    Lê um arquivo .conllu e retorna um DataFrame com as colunas principais.
    """
    linhas = []
    with open(caminho, 'r', encoding='utf-8') as f:
        for tokenlist in parse_incr(f): 
            for token in tokenlist:
                linhas.append({
                    "sent_id": tokenlist.metadata.get("sent_id", None),
                    "form": token.get("form"),
                    "lemma": token.get("lemma"),
                    "upos": token.get("upos"),
                    "id": token.get("id")
                })

    df = pd.DataFrame(linhas)
    return df

# Teste
# df_train = ler_conllu("pt_porttinari-ud-train.conllu")
# print(df_train.head())


def filtrar_multiword(df):
    """
    Remove tokens que representam multiwords (id como '3-4') do DataFrame.
    """
    # No formato CoNLL-U, multiwords têm id string com hífen, ex: '3-4'
    df_filtrado = df[~df["id"].astype(str).str.contains("-")].copy()
    return df_filtrado

# Teste
# df_train_filtered = filtrar_multiword(df_train)
# print(df_train_filtered[38:48])

def extrair_colunas_parteA(df):
    """
    Retorna uma lista de sentenças, onde cada sentença é uma lista de palavras (forms).
    Transforma todas as palavras em minúsculas.
    """
    sentencas = []
    for _, grupo in df.groupby("sent_id"):
        palavras = [w.lower() for w in grupo["form"].tolist()]
        sentencas.append(palavras)
    return sentencas


def extrair_colunas_parteB(df):
    """
    Retorna uma lista de sentenças, onde cada sentença é uma lista de tuplas (palavra, tag).
    Transforma todas as palavras em minúsculas.
    """
    sentencas_tags = []
    for _, grupo in df.groupby("sent_id"):
        pares = [(w.lower(), t) for w, t in zip(grupo["form"], grupo["upos"])]
        sentencas_tags.append(pares)
    return sentencas_tags

# parteA_train = extrair_colunas_parteA(df_train_filtered)
# parteB_train = extrair_colunas_parteB(df_train_filtered)

# print(parteA_train[0])
# print(parteB_train[0])
