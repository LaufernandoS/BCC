import pandas as pd
import collections
from typing import List, Dict, Any

class BigramModel:
    def __init__(self, lambda_smooth=1.0):
        # Estrutura esparsa (defaultdict(Counter)): só guarda os que aparecem → O(#bigramas) memória.
        self.unigram_counts = collections.Counter()
        self.bigram_counts = collections.defaultdict(collections.Counter)
        self.vocabulary = set()
        self.lambda_smooth = lambda_smooth
        self.prob_cache = {}

        self.total_bigrams = 0  # Total de bigramas contados
        self.total_unigrams = 0  # Total de unigramas contados

    def treinar(self, sentencas):
        """Treina o modelo de bigramas com as sentenças fornecidas."""
        for sent in sentencas:
            # Transforma em minúsculas e adiciona marcadores de início e fim
            sent_completa = ['<s>'] + sent + ['</s>']
            
            # Atualiza contagens de unigramas
            self.unigram_counts.update(sent_completa)
            self.vocabulary.update(sent_completa)
            
            # Atualiza contagens de bigramas
            bigrams = list(zip(sent_completa[:-1], sent_completa[1:]))
            for w1, w2 in bigrams:
                self.bigram_counts[w1][w2] += 1
            
            self.total_bigrams += len(bigrams)
        
        # Total de unigramas
        self.total_unigrams = sum(self.unigram_counts.values())
        
        # Remove <s> do vocabulário (não pode ser previsto)
        if '<s>' in self.vocabulary:
            self.vocabulary.remove('<s>')
        
        # Limpa cache
        self.prob_cache.clear()
    
    def identificar_desconhecidas(self, sentencas_val):
        """Identifica palavras desconhecidas no corpus de validação."""
        vocab_train = self.vocabulary
        palavras_desc = set()
        for sent in sentencas_val:
            for palavra in sent:
                if palavra not in vocab_train and palavra not in ['<s>', '</s>']:
                    palavras_desc.add(palavra)
        return palavras_desc
    
    def processar_desconhecidas(self, sentencas_val):
        """
        Substitui palavras desconhecidas por <DESC> e calcula estatísticas.
        IMPORTANTE: Não modifica as contagens de treino originais.
        """
        palavras_desc = self.identificar_desconhecidas(sentencas_val)
        if not palavras_desc:
            return sentencas_val  # Nada a fazer
        
        # Adiciona <DESC> ao vocabulário
        self.vocabulary.add("<DESC>")
        
        # Substitui palavras desconhecidas
        sentencas_processadas = []
        for sent in sentencas_val:
            nova_sentenca = [
                palavra if palavra in self.vocabulary or palavra in ['<s>', '</s>'] 
                else "<DESC>"
                for palavra in sent
            ]
            sentencas_processadas.append(nova_sentenca)
        
        # Calcula contagens de bigramas envolvendo <DESC> no corpus de validação
        desc_bigram_counts = collections.defaultdict(collections.Counter)
        desc_unigram_counts = collections.Counter()
        
        for sent in sentencas_processadas:
            sent_completa = ['<s>'] + sent + ['</s>']
            desc_unigram_counts.update(sent_completa)
            
            bigrams = zip(sent_completa[:-1], sent_completa[1:])
            for w1, w2 in bigrams:
                if "<DESC>" in (w1, w2):
                    desc_bigram_counts[w1][w2] += 1
        
        # Atualiza contagens do modelo com informações de <DESC>
        # (fazemos isso para que as probabilidades P(w|<DESC>) sejam calculáveis)
        for w1 in desc_bigram_counts:
            for w2 in desc_bigram_counts[w1]:
                self.bigram_counts[w1][w2] += desc_bigram_counts[w1][w2]
        
        for palavra, count in desc_unigram_counts.items():
            self.unigram_counts[palavra] += count
        
        # Limpa o cache após atualizar contagens
        self.prob_cache.clear()
        
        return sentencas_processadas
    
    def calcular_probabilidade(self, w1, w2):
        """
        Calcula P(w2|w1) com suavização de Lidstone:
        P(w2|w1) = (C(w1,w2) + λ) / (C(w1) + V*λ)
        onde V é o tamanho do vocabulário.
        """
        # Verifica cache
        if (w1, w2) in self.prob_cache:
            return self.prob_cache[(w1, w2)]
        
        vocab_size = len(self.vocabulary)
        count_w1_w2 = self.bigram_counts.get(w1, {}).get(w2, 0)
        count_w1 = self.unigram_counts.get(w1, 0)
        
        # Aplicando Lidstone corretamente
        # P(w2|w1) = (C(w1,w2) + λ) / (C(w1) + V*λ)
        prob = (count_w1_w2 + self.lambda_smooth) / (count_w1 + vocab_size * self.lambda_smooth)
        
        self.prob_cache[(w1, w2)] = prob
        return prob

    def prever_proxima(self, palavra_atual):
        """
        Retorna a palavra mais provável após 'palavra_atual' com base nas contagens.
        """
        candidatos = self.bigram_counts.get(palavra_atual, {})
        if candidatos:
            # Usa apenas bigramas já vistos com essa palavra
            return max(candidatos.items(), key=lambda x: x[1])[0]
        # Palavra nunca vista / sem sucessor
        return "</s>"


    def prever_sentenca(self, sentenca_gold: List[str]) -> List[str]:
        """
        Prevê uma sentença completa usando o método descrito no enunciado:
        - Começa com <s> e prevê a primeira palavra
        - Usa a palavra CORRETA do gold standard para prever a próxima
        - Continua até ter previsto o mesmo número de palavras (incluindo </s>)
        
        Retorna a lista de palavras previstas (incluindo </s> ao final)
        GARANTE que len(predicoes) == len(sentenca_gold) + 1 (com o </s>)
        """
        predicoes = []
        palavra_atual = '<s>'
        
        # Prevê cada palavra da sentença
        for palavra_gold in sentenca_gold:
            # Prevê a próxima palavra baseado na atual
            palavra_prevista = self.prever_proxima(palavra_atual)
            predicoes.append(palavra_prevista)
            
            # IMPORTANTE: usa a palavra CORRETA (gold) para continuar
            palavra_atual = palavra_gold
        
        # Prevê o marcador de fim </s>
        palavra_prevista = self.prever_proxima(palavra_atual)
        predicoes.append(palavra_prevista)
        
        return predicoes

def avaliar(gold: List[List[str]], predicted: List[List[str]]) -> Dict[str, Any]:
    """
    Avalia as previsões comparando com o padrão ouro.
    
    Args:
        gold: Corpus de teste (padrão ouro) - lista de sentenças
        predicted: Corpus de previsões - lista de sentenças (deve incluir </s>)
    
    Returns:
        Dict com métricas: precision, recall, f-score por palavra e accuracy global
    """
    # Para cada sentença, o gold tem N palavras e o predicted tem N+1 (incluindo </s>)
    # Precisamos comparar: predicted[0:N] com gold, e predicted[N] deve ser </s>
    
    gold_flat = []
    pred_flat = []
    
    for sent_gold, sent_pred in zip(gold, predicted):
        # sent_gold: ['o', 'gato', 'dormiu']
        # sent_pred: ['o', 'gato', 'dormiu', '</s>']  (esperado)
        
        # Adiciona as palavras da sentença
        gold_flat.extend(sent_gold)
        pred_flat.extend(sent_pred[:len(sent_gold)])
        
        # Adiciona o </s>
        gold_flat.append('</s>')
        pred_flat.append(sent_pred[len(sent_gold)] if len(sent_pred) > len(sent_gold) else '</s>')
    
    # Agora gold_flat e pred_flat têm o mesmo tamanho
    total_palavras = len(gold_flat)
    
    # Contadores para cada palavra
    word_correct = collections.Counter()  # Ocorrências corretas de cada palavra
    word_pred_total = collections.Counter()  # Total de ocorrências no corpus de previsão
    word_gold_total = collections.Counter()  # Total de ocorrências no corpus de teste
    
    palavras_corretas = 0
    
    # Calcula contagens
    for g, p in zip(gold_flat, pred_flat):
        word_gold_total[g] += 1
        word_pred_total[p] += 1
        
        if g == p:
            word_correct[g] += 1
            palavras_corretas += 1
    
    # Calcula métricas por palavra
    vocabulario = set(gold_flat) | set(pred_flat)
    
    metricas_por_palavra = {}
    for w in vocabulario:
        corretas = word_correct.get(w, 0)
        total_pred = word_pred_total.get(w, 0)
        total_gold = word_gold_total.get(w, 0)
        
        # Precision: corretas / preditas
        precision = corretas / total_pred if total_pred > 0 else 0.0
        
        # Recall: corretas / gold
        recall = corretas / total_gold if total_gold > 0 else 0.0
        
        # F-score: média harmônica
        if precision + recall > 0:
            f_score = 2 * (precision * recall) / (precision + recall)
        else:
            f_score = 0.0
        
        metricas_por_palavra[w] = {
            'precision': precision,
            'recall': recall,
            'f_score': f_score,
            'corretas': corretas,
            'total_pred': total_pred,
            'total_gold': total_gold
        }
    
    # Accuracy global
    accuracy = palavras_corretas / total_palavras if total_palavras > 0 else 0.0
    
    return {
        'accuracy': accuracy,
        'metricas_por_palavra': metricas_por_palavra, 
        'total_palavras': total_palavras,
        'palavras_corretas': palavras_corretas
    }

def metricas_por_palavra(sentencas_gold, sentencas_pred):
    """
    Calcula precision, recall e F1-score por palavra.
    Retorna um DataFrame com colunas: palavra, precision, recall, f1, suporte.
    """
    tp = collections.Counter()   # acertos por palavra
    fp = collections.Counter()   # falsos positivos
    fn = collections.Counter()   # falsos negativos

    for gold, pred in zip(sentencas_gold, sentencas_pred):
        for g, p in zip(gold, pred):
            if g == p:
                tp[g] += 1
            else:
                fp[p] += 1
                fn[g] += 1

    palavras = set(tp.keys()) | set(fp.keys()) | set(fn.keys())
    metricas = []

    for w in palavras:
        prec = tp[w] / (tp[w] + fp[w]) if (tp[w] + fp[w]) > 0 else 0
        rec = tp[w] / (tp[w] + fn[w]) if (tp[w] + fn[w]) > 0 else 0
        f1 = 2 * prec * rec / (prec + rec) if (prec + rec) > 0 else 0
        suporte = tp[w] + fn[w]
        metricas.append((w, prec, rec, f1, suporte))

    df = pd.DataFrame(metricas, columns=["palavra", "precision", "recall", "f1", "suporte"])
    return df.sort_values("f1", ascending=False).reset_index(drop=True)
