import pandas as pd
import numpy as np
from collections import defaultdict, Counter

class HMM:
    def __init__(self, lambda_smooth=0.01, eps=1e-3):
        self.tags = ['ADJ', 'ADP', 'ADV', 'AUX', 'CCONJ', 'DET', 'INTJ', 'NOUN',
                'NUM', 'PART', 'PRON', 'PROPN', 'PUNCT', 'SCONJ', 'SYM', 'VERB', 'X']
        
        # Inicialização (será refeita no treino)
        self.A = np.ones((289, 17)) / 17
        self.B = np.ones((17, 1))
        self.pi = np.ones(289) / 289
        
        self.state_names = [(t1, t2) for t1 in self.tags for t2 in self.tags]
        self.vocabulary = {}
        self.lambda_smooth = lambda_smooth
        self.eps = eps
        self.eps_log = 1e-8   # estabilidade numérica em log()

    def _criar_mapeamentos(self, sentencas_treino):
        vocab = sorted({w for sentenca in sentencas_treino for (w, t) in sentenca})
        self.vocabulary = {w: i for i, w in enumerate(vocab)}
        self.palavra2idx = self.vocabulary
        self.tag2idx = {t: i for i, t in enumerate(self.tags)}
        self.estado2idx = {pair: i for i, pair in enumerate(self.state_names)}
        
    def inicializar_modelo(self, sentencas_treino):
        self._criar_mapeamentos(sentencas_treino)
        V = len(self.vocabulary)
        N_tags = len(self.tags)
        N_states = len(self.state_names)

        # Smoothing inicial
        contagem_A = np.full((N_states, N_tags), self.lambda_smooth)
        contagem_pi = np.full(N_states, self.lambda_smooth)
        contagem_B = np.full((N_tags, V), self.lambda_smooth)

        for sent in sentencas_treino:
            tags = [t for (w, t) in sent]
            
            # Pi: primeiro par de tags
            if len(tags) >= 2:
                estado_inicial = (tags[0], tags[1])
                contagem_pi[self.estado2idx[estado_inicial]] += 1

            # Transições: (tag_i, tag_i+1) -> tag_i+2
            for i in range(len(tags) - 2):
                par_atual = (tags[i], tags[i+1])
                prox_tag = tags[i+2]
                contagem_A[self.estado2idx[par_atual], self.tag2idx[prox_tag]] += 1

            # Emissões
            for w, t in sent:
                if w in self.vocabulary:
                    contagem_B[self.tag2idx[t], self.palavra2idx[w]] += 1

        self.A = contagem_A / contagem_A.sum(axis=1, keepdims=True)
        self.pi = contagem_pi / contagem_pi.sum()
        self.B = contagem_B / contagem_B.sum(axis=1, keepdims=True)

    def forward(self, sentenca):
        """
        Forward para HMM de 2ª ordem.
        Alpha[t, estado] = P(obs[0:t+1], estado_t = estado)
        Estado = (tag_{t-1}, tag_t)
        """
        T = len(sentenca)
        N_states = len(self.state_names)
        N_tags = len(self.tags)
        
        # Log-space para estabilidade
        log_alpha = np.full((T, N_states), -np.inf)
        
        # Inicialização (t=1, pois precisamos de 2 tags)
        if T < 2:
            raise ValueError("Sentença precisa ter pelo menos 2 palavras")
        
        w0_idx = self.palavra2idx.get(sentenca[0], None)
        w1_idx = self.palavra2idx.get(sentenca[1], None)
        
        for s_idx, (tag0, tag1) in enumerate(self.state_names):
            # P(estado inicial) × P(w0|tag0) × P(w1|tag1)
            emis0 = self.B[self.tag2idx[tag0], w0_idx] if w0_idx is not None else self.lambda_smooth
            emis1 = self.B[self.tag2idx[tag1], w1_idx] if w1_idx is not None else self.lambda_smooth
            
            log_alpha[1, s_idx] = (np.log(self.pi[s_idx] + self.eps_log) + 
                                   np.log(emis0 + self.eps_log) + 
                                   np.log(emis1 + self.eps_log))
        
        # Recorrência para t >= 2
        for t in range(2, T):
            w_idx = self.palavra2idx.get(sentenca[t], None)
            
            for s_idx, (tag_prev, tag_curr) in enumerate(self.state_names):
                # Estado atual é (tag_prev, tag_curr)
                # Transição: estados anteriores (tag_x, tag_prev) -> tag_curr
                
                emis = self.B[self.tag2idx[tag_curr], w_idx] if w_idx is not None else self.lambda_smooth
                log_emis = np.log(emis + self.eps_log)
                
                # Soma sobre todos os estados anteriores que terminam com tag_prev
                log_sum = -np.inf
                for prev_s_idx, (tag_x, tag_y) in enumerate(self.state_names):
                    if tag_y == tag_prev:  # Compatível: (tag_x, tag_prev) -> (tag_prev, tag_curr)
                        trans_prob = self.A[prev_s_idx, self.tag2idx[tag_curr]]
                        log_prob = log_alpha[t-1, prev_s_idx] + np.log(trans_prob + self.eps_log)
                        log_sum = np.logaddexp(log_sum, log_prob)
                
                log_alpha[t, s_idx] = log_emis + log_sum
        
        return log_alpha

    def backward(self, sentenca):
        """
        Backward para HMM de 2ª ordem.
        Beta[t, estado] = P(obs[t+1:T] | estado_t = estado)
        """
        T = len(sentenca)
        N_states = len(self.state_names)
        
        log_beta = np.zeros((T, N_states))  # Log(1) = 0
        
        # Recorrência reversa
        for t in range(T - 2, 0, -1):  # De T-2 até 1
            w_next_idx = self.palavra2idx.get(sentenca[t+1], None)
            
            for s_idx, (tag_prev, tag_curr) in enumerate(self.state_names):
                # Estado atual: (tag_prev, tag_curr)
                # Próximos estados: (tag_curr, tag_z) para qualquer z
                
                log_sum = -np.inf
                for next_s_idx, (tag_y, tag_z) in enumerate(self.state_names):
                    if tag_y == tag_curr:  # Compatível
                        trans_prob = self.A[s_idx, self.tag2idx[tag_z]]
                        emis = self.B[self.tag2idx[tag_z], w_next_idx] if w_next_idx is not None else self.lambda_smooth
                        
                        log_prob = (np.log(trans_prob + self.eps_log) + 
                                   np.log(emis + self.eps_log) + 
                                   log_beta[t+1, next_s_idx])
                        log_sum = np.logaddexp(log_sum, log_prob)
                
                log_beta[t, s_idx] = log_sum
        
        return log_beta

    def calcular_gamma(self, log_alpha, log_beta):
        """
        Gamma[t, estado] = P(estado_t = estado | O)
        """
        log_gamma = log_alpha + log_beta
        # Normaliza (em log-space); retira warning de invalid value por valor -inf
        with np.errstate(invalid='ignore'):
            log_gamma -= np.max(log_gamma, axis=1, keepdims=True)
        gamma = np.exp(log_gamma)
        gamma /= gamma.sum(axis=1, keepdims=True)
        return gamma

    def calcular_xi(self, sentenca, log_alpha, log_beta):
        """
        Xi[t, estado_atual, tag_proxima] = P(estado_t, estado_{t+1} | O)
        Para HMM 2ª ordem: transição de (x,y) para (y,z)
        """
        T = len(sentenca)
        N_states = len(self.state_names)
        N_tags = len(self.tags)
        
        xi = np.zeros((T-1, N_states, N_tags))  # (tempo, estado_atual, prox_tag)
        
        for t in range(1, T - 1):  # De 1 a T-2
            w_next_idx = self.palavra2idx.get(sentenca[t+1], None)
            
            log_xi = np.full((N_states, N_tags), -np.inf)
            
            for s_idx, (tag_prev, tag_curr) in enumerate(self.state_names):
                for z_idx, tag_z in enumerate(self.tags):
                    # Transição: (tag_prev, tag_curr) -> (tag_curr, tag_z)
                    next_state_idx = self.estado2idx[(tag_curr, tag_z)]
                    
                    trans_prob = self.A[s_idx, z_idx]
                    emis = self.B[z_idx, w_next_idx] if w_next_idx is not None else self.lambda_smooth
                    
                    log_xi[s_idx, z_idx] = (log_alpha[t, s_idx] + 
                                            np.log(trans_prob + self.eps_log) + 
                                            np.log(emis + self.eps_log) + 
                                            log_beta[t+1, next_state_idx])
            
            # Normaliza
            log_xi -= np.max(log_xi)
            xi[t] = np.exp(log_xi)
            xi[t] /= xi[t].sum()
        
        return xi

    def atualizar_modelo(self, gammas, xis, sentencas):
        """
        Atualiza A, B, pi usando gammas e xis
        """
        N_states = len(self.state_names)
        N_tags = len(self.tags)
        V = len(self.vocabulary)
        
        # Atualiza Pi (média dos gammas no tempo t=1)
        pi_new = np.mean([g[1] for g in gammas], axis=0)  # t=1 pois é onde calculamos primeiro
        
        # Atualiza A
        A_num = np.zeros((N_states, N_tags))
        A_den = np.zeros(N_states)
        
        for xi_seq in xis:
            for t in range(len(xi_seq)):
                A_num += xi_seq[t]  # Soma sobre todos os tempos
                A_den += xi_seq[t].sum(axis=1)
        
        A_new = A_num / (A_den[:, None] + self.eps_log)
        
        # Atualiza B
        B_num = np.zeros((N_tags, V))
        B_den = np.zeros(N_tags)
        
        for seq_idx, sent in enumerate(sentencas):
            gamma_seq = gammas[seq_idx]
            
            for t in range(1, len(sent)):  # Começando de t=1
                w, tag = sent[t]
                w_idx = self.palavra2idx.get(w, None)
                
                if w_idx is not None:
                    # Marginaliza sobre estados que contêm essa tag na posição correta
                    for s_idx, (tag_prev, tag_curr) in enumerate(self.state_names):
                        if tag_curr == tag:
                            B_num[self.tag2idx[tag], w_idx] += gamma_seq[t, s_idx]
                            B_den[self.tag2idx[tag]] += gamma_seq[t, s_idx]
        
        B_new = B_num / (B_den[:, None] + self.eps_log)
        
        return A_new, B_new, pi_new

    def calcular_distancia(self, A_new, B_new, pi_new):
        return (np.linalg.norm(self.A - A_new) + 
                np.linalg.norm(self.B - B_new) + 
                np.linalg.norm(self.pi - pi_new))

    def treinar(self, sentencas_treino, max_iter=10, eps=1e-3):
        self.inicializar_modelo(sentencas_treino)
        T = len(sentencas_treino)
        print("Inicialização concluída. Iniciando EM...")

        for it in range(max_iter):
            gammas, xis = [], []

            for s_idx, sent in enumerate(sentencas_treino):
                palavras = [w for (w, t) in sent]
                
                if len(palavras) < 2:
                    continue
                
                try:
                    log_alpha = self.forward(palavras)
                    log_beta = self.backward(palavras)
                    gamma = self.calcular_gamma(log_alpha, log_beta)
                    xi = self.calcular_xi(palavras, log_alpha, log_beta)
                    
                    gammas.append(gamma)
                    xis.append(xi)
                except Exception as e:
                    print(f"Erro ao processar sentença: {e}")
                    continue

                if (s_idx + 1) % 1000 == 0 or (s_idx + 1) == T:
                    print(f"  Processadas {s_idx + 1}/{T} sentenças")

            A_new, B_new, pi_new = self.atualizar_modelo(gammas, xis, sentencas_treino)
            dist = self.calcular_distancia(A_new, B_new, pi_new)
            print(f"Iteração {it+1}: distância = {dist:.6f}")
            
            self.A, self.B, self.pi = A_new, B_new, pi_new

            if dist < eps:
                print("Convergência alcançada.")
                break
            
            # Verifica se quer continuar o treinamento
            if input("Continuar? (s/n): ").strip().lower() == 'n':
                break

    def processar_desconhecidas(self, sentencas_val):
        """
        Adiciona coluna <DESC> na matriz B para palavras desconhecidas.
        
        Passos:
        1. Adiciona nova coluna à matriz B com probabilidades baseadas em lambda_smooth
        2. Normaliza as probabilidades para manter soma das linhas = 1
        3. Atualiza o vocabulário com o token <DESC>
        
        Args:
            sentencas_val: lista de listas de tuplas (palavra, tag) para validação
        """

        N_tags = len(self.tags)
        V_old = len(self.vocabulary)
        
        print("Processando palavras desconhecidas...")
        
        # Para calcular probabilidade para <DESC> usa lambda_smooth como base, similar à inicialização
        desc_probs = np.full((N_tags, 1), self.lambda_smooth)
        
        # Adiciona coluna <DESC> à matriz B
        self.B = np.hstack([self.B, desc_probs])
        
        # Normalização
        self.B = self.B / self.B.sum(axis=1, keepdims=True)
        
        # Atualiza vocabulário
        self.vocabulary['<DESC>'] = V_old
        self.palavra2idx['<DESC>'] = V_old
        
        # Identifica palavras desconhecidas nas sentenças de validação
        palavras_desc = set()
        for sent in sentencas_val:
            for palavra, _ in sent:
                if palavra not in self.vocabulary or palavra == '<DESC>':
                    palavras_desc.add(palavra)

        print(f"Total de palavras desconhecidas encontradas: {len(palavras_desc)}")
        
    def viterbi(self, sentenca):
        """
        Algoritmo de Viterbi para HMM de 2ª ordem.
        
        Encontra a sequência de tags mais provável para a sentença.
        Palavras desconhecidas são substituídas por <DESC> automaticamente.
        
        Args:
            sentenca: lista de palavras (strings)
        
        Returns:
            lista de tags (strings) com o mesmo tamanho da sentença
        """
        T = len(sentenca)
        
        if T < 2:
            raise ValueError("Sentença precisa ter pelo menos 2 palavras")
        
        # Substitui palavras desconhecidas por <DESC>
        sentenca_processada = []
        for palavra in sentenca:
            if palavra in self.palavra2idx:
                sentenca_processada.append(palavra)
            else:
                sentenca_processada.append('<DESC>')
        
        N_states = len(self.state_names)
        
        # Matrizes de Viterbi
        # delta[t, s] = max probabilidade de sequência até t terminando no estado s
        log_delta = np.full((T, N_states), -np.inf)
        
        # psi[t, s] = estado anterior que maximiza delta[t, s]
        psi = np.zeros((T, N_states), dtype=int)
        
        # Inicialização (t=1)
        w0_idx = self.palavra2idx.get(sentenca_processada[0])
        w1_idx = self.palavra2idx.get(sentenca_processada[1])
        
        for s_idx, (tag0, tag1) in enumerate(self.state_names):
            # P(estado inicial) × P(w0|tag0) × P(w1|tag1)
            emis0 = self.B[self.tag2idx[tag0], w0_idx]
            emis1 = self.B[self.tag2idx[tag1], w1_idx]
            
            log_delta[1, s_idx] = (np.log(self.pi[s_idx] + self.eps_log) + 
                                np.log(emis0 + self.eps_log) + 
                                np.log(emis1 + self.eps_log))
        
        # Recorrência (t >= 2)
        for t in range(2, T):
            w_idx = self.palavra2idx.get(sentenca_processada[t])
            
            for s_idx, (tag_prev, tag_curr) in enumerate(self.state_names):
                # Estado atual: (tag_prev, tag_curr)
                # Precisamos encontrar o melhor estado anterior: (tag_x, tag_prev)
                
                emis = self.B[self.tag2idx[tag_curr], w_idx]
                log_emis = np.log(emis + self.eps_log)
                
                max_prob = -np.inf
                best_prev_state = 0
                
                # Busca sobre todos os estados anteriores compatíveis
                for prev_s_idx, (tag_x, tag_y) in enumerate(self.state_names):
                    if tag_y == tag_prev:  # Compatibilidade: (tag_x, tag_prev) → (tag_prev, tag_curr)
                        trans_prob = self.A[prev_s_idx, self.tag2idx[tag_curr]]
                        
                        prob = log_delta[t-1, prev_s_idx] + np.log(trans_prob + self.eps_log)
                        
                        if prob > max_prob:
                            max_prob = prob
                            best_prev_state = prev_s_idx
                
                log_delta[t, s_idx] = log_emis + max_prob
                psi[t, s_idx] = best_prev_state
        
        # Backtracking: encontra a melhor sequência de estados
        best_last_state = np.argmax(log_delta[T-1, :])
        
        # Reconstrói a sequência de estados
        state_sequence = [0] * T
        state_sequence[T-1] = best_last_state
        
        for t in range(T-2, 0, -1):  # De T-2 até 1
            state_sequence[t] = psi[t+1, state_sequence[t+1]]
        
        # Extrai as tags da sequência de estados
        tags = []
        
        # Primeira tag: vem do primeiro estado (tag0, tag1)
        first_state = self.state_names[state_sequence[1]]
        tags.append(first_state[0])  # tag0
        
        # Tags restantes: segunda componente de cada estado
        for t in range(1, T):
            state = self.state_names[state_sequence[t]]
            tags.append(state[1])  # tag1 do par (tag0, tag1)
        
        return tags
    
    def etiquetar_corpus(self, sentencas_teste, verbose=False):
        """
        Etiqueta todas as sentenças do corpus de teste usando Viterbi.
        
        Args:
            sentencas_teste: lista de listas de tuplas (palavra, tag_gold)
            verbose: se True, mostra progresso
        
        Returns:
            lista de tuplas (palavra, tag_predita) para todas as sentenças
        """
        resultados = []
        total = len(sentencas_teste)
        
        if verbose:
            print(f"Iniciando etiquetagem de {total} sentenças...")
        
        for i, sentenca in enumerate(sentencas_teste):
            # Extrai apenas as palavras (ignora as tags gold)
            palavras = [w for (w, t) in sentenca]
            
            try:
                # Etiqueta usando Viterbi
                tags_preditas = self.viterbi(palavras)
                
                # Cria lista de tuplas (palavra, tag_predita)
                sentenca_etiquetada = list(zip(palavras, tags_preditas))
                resultados.append(sentenca_etiquetada)
                
            except Exception as e:
                if verbose:
                    print(f"Erro na sentença {i}: {e}")
                # Em caso de erro, retorna tags 'X' (desconhecido)
                sentenca_etiquetada = [(w, 'X') for w in palavras]
                resultados.append(sentenca_etiquetada)
            
            # Mostra progresso
            if verbose and (i + 1) % 100 == 0:
                print(f"  Processadas {i + 1}/{total} sentenças")
        
        if verbose:
            print(f"Etiquetagem concluída: {total} sentenças processadas.")
        
        return resultados

def exportar_csv(hmm, prefixo_arquivo="hmm_iter_updated"):
    """
    Exporta as matrizes A, B e o vetor pi de um modelo HMM para arquivos CSV.

    Parâmetros:
    -----------
    hmm : objeto HMM
        Instância já treinada contendo os atributos .A, .B e .pi.
    prefixo_arquivo : str, opcional
        Prefixo usado no nome dos arquivos gerados.
        Serão criados: prefixo_arquivo_A.csv, prefixo_arquivo_B.csv, prefixo_arquivo_pi.csv

    Retorno:
    --------
    None
    """
    # === Validações básicas ===
    if not hasattr(hmm, "A") or not hasattr(hmm, "B") or not hasattr(hmm, "pi"):
        raise AttributeError("O objeto HMM precisa conter os atributos A, B e pi.")

    # === Matriz de transição A ===
    df_A = pd.DataFrame(
        hmm.A,
        index=[f"{t1},{t2}" for (t1, t2) in hmm.state_names],
        columns=hmm.tags
    )
    df_A.to_csv(f"{prefixo_arquivo}_A.csv", index_label="estado")

    # === Matriz de emissão B ===
    df_B = pd.DataFrame(
        hmm.B,
        index=hmm.tags,
        columns=hmm.vocabulary
    )
    df_B.to_csv(f"{prefixo_arquivo}_B.csv", index_label="tag")

    # === Vetor de probabilidades iniciais pi ===
    df_pi = pd.DataFrame(
        {"estado": [f"{t1},{t2}" for (t1, t2) in hmm.state_names], "pi": hmm.pi}
    )
    df_pi.to_csv(f"{prefixo_arquivo}_pi.csv", index=False)

    print(f"Modelos exportados como:\n - {prefixo_arquivo}_A.csv\n - {prefixo_arquivo}_B.csv\n - {prefixo_arquivo}_pi.csv")

def importar_csv(hmm, prefixo_arquivo="hmm_iter"):
    """
    Importa matrizes A, B e o vetor pi de um modelo HMM salvo em arquivos CSV.

    Parâmetros:
    -----------
    hmm : objeto HMM
        Instância contendo os atributos .A, .B e .pi.
    prefixo_arquivo : str, opcional
        Prefixo usado no nome dos arquivos gerados.
        Serão exigidos: prefixo_arquivo_A.csv, prefixo_arquivo_B.csv, prefixo_arquivo_pi.csv

    Retorno:
    --------
    None
    """
    # Matriz de transição A 
    df_A = pd.read_csv(f"{prefixo_arquivo}_A.csv", index_col="estado")
    hmm.A = df_A.values

    # Matriz de emissão B 
    df_B = pd.read_csv(f"{prefixo_arquivo}_B.csv", index_col="tag")
    hmm.B = df_B.values
    hmm.vocabulary = list(df_B.columns)

    # Vetor de probabilidades iniciais pi
    df_pi = pd.read_csv(f"{prefixo_arquivo}_pi.csv")
    hmm.pi = df_pi["pi"].values

    # Importa o vocabulário e faz mapeamentos necessários
    hmm.vocabulary = {palavra: i for i, palavra in enumerate(df_B.columns)}
    hmm.palavra2idx = hmm.vocabulary
    hmm.tag2idx = {t: i for i, t in enumerate(hmm.tags)}
    hmm.estado2idx = {pair: i for i, pair in enumerate(hmm.state_names)}

    print(f"Modelos importados de:\n - {prefixo_arquivo}_A.csv\n - {prefixo_arquivo}_B.csv\n - {prefixo_arquivo}_pi.csv")

def avaliar_etiquetagem(sentencas_gold, sentencas_pred, tags_possiveis=None):
    """
    Avalia a etiquetagem comparando padrão ouro com predições.
    
    Args:
        sentencas_gold: lista de listas de tuplas (palavra, tag_gold)
        sentencas_pred: lista de listas de tuplas (palavra, tag_pred)
        tags_possiveis: lista de tags (default: extrai das sentenças)
    
    Returns:
        dict com métricas: metricas_por_tag, macro_avg, weighted_avg, 
                          accuracy, top10_dificeis
    """
    # Extrai tags se não fornecido
    if tags_possiveis is None:
        tags_possiveis = sorted(set(tag for sent in sentencas_gold for (_, tag) in sent))
    
    # Contadores
    tp = defaultdict(int)
    fp = defaultdict(int)
    fn = defaultdict(int)
    total_palavras = 0
    acertos = 0
    erros_por_palavra = defaultdict(lambda: {'total': 0, 'erros': 0, 'confusoes': []})
    
    # Processa sentenças
    for sent_gold, sent_pred in zip(sentencas_gold, sentencas_pred):
        for (palavra, tag_gold), (_, tag_pred) in zip(sent_gold, sent_pred):
            total_palavras += 1
            
            if tag_gold == tag_pred:
                tp[tag_gold] += 1
                acertos += 1
            else:
                fn[tag_gold] += 1
                fp[tag_pred] += 1
            
            # Rastreamento de erros por palavra; armazena por palavra os erros de tags / confusões
            erros_por_palavra[palavra]['total'] += 1
            if tag_gold != tag_pred:
                erros_por_palavra[palavra]['erros'] += 1
                erros_por_palavra[palavra]['confusoes'].append((tag_gold, tag_pred))
    
    # Métricas por tag
    metricas_por_tag = {}
    for tag in tags_possiveis:
        precision = tp[tag] / (tp[tag] + fp[tag]) if (tp[tag] + fp[tag]) > 0 else 0.0 # Das vezes que previmos X, quantas acertamos?
        recall = tp[tag] / (tp[tag] + fn[tag]) if (tp[tag] + fn[tag]) > 0 else 0.0    # Das vezes que era X, quantas computamos?
        f1 = 2 * precision * recall / (precision + recall) if (precision + recall) > 0 else 0.0
        
        metricas_por_tag[tag] = {
            'precision': precision,
            'recall': recall,
            'f1': f1,
            'suporte': tp[tag] + fn[tag] # Usado para weighted average
        }
    
    # Macro average
    macro_precision = np.mean([m['precision'] for m in metricas_por_tag.values()])
    macro_recall = np.mean([m['recall'] for m in metricas_por_tag.values()])
    macro_f1 = np.mean([m['f1'] for m in metricas_por_tag.values()])
    f1_from_avg = 2 * macro_precision * macro_recall / (macro_precision + macro_recall) \
                  if (macro_precision + macro_recall) > 0 else 0.0
    
    # Weighted average
    total_suporte = sum(m['suporte'] for m in metricas_por_tag.values())
    weighted_precision = sum(m['precision'] * m['suporte'] for m in metricas_por_tag.values()) / total_suporte
    weighted_recall = sum(m['recall'] * m['suporte'] for m in metricas_por_tag.values()) / total_suporte
    weighted_f1 = sum(m['f1'] * m['suporte'] for m in metricas_por_tag.values()) / total_suporte
    
    # Acurácia
    accuracy = acertos / total_palavras if total_palavras > 0 else 0.0
    
    
    return {
        'metricas_por_tag': metricas_por_tag,
        'macro_avg': {
            'precision': macro_precision,
            'recall': macro_recall,
            'f1': macro_f1,
            'f1_from_avg': f1_from_avg
        },
        'weighted_avg': {
            'precision': weighted_precision,
            'recall': weighted_recall,
            'f1': weighted_f1
        },
        'accuracy': accuracy,
        'total_palavras': total_palavras,
        'acertos': acertos,
    }

def imprimir_relatorio(resultados):
    """
    Imprime relatório formatado das métricas (opcional).
    
    Args:
        resultados: dict retornado por avaliar_etiquetagem()
    """
    print("\n" + "="*60)
    print("RELATÓRIO DE AVALIAÇÃO")
    print("="*60)
    
    # Acurácia
    print(f"\nAcurácia: {resultados['accuracy']:.4f} ({resultados['accuracy']*100:.2f}%)")
    print(f"Acertos: {resultados['acertos']}/{resultados['total_palavras']}")
    
    # Macro average
    macro = resultados['macro_avg']
    print(f"\nMacro Average:")
    print(f"  Precision: {macro['precision']:.4f}")
    print(f"  Recall:    {macro['recall']:.4f}")
    print(f"  F1:        {macro['f1']:.4f}")
    print(f"  F1 (P+R):  {macro['f1_from_avg']:.4f}")
    
    # Weighted average
    weighted = resultados['weighted_avg']
    print(f"\nWeighted Average:")
    print(f"  Precision: {weighted['precision']:.4f}")
    print(f"  Recall:    {weighted['recall']:.4f}")
    print(f"  F1:        {weighted['f1']:.4f}")
    
    # Métricas por tag (resumo)
    print(f"\nMétricas por tag (top 5 piores F1):")
    piores = sorted(resultados['metricas_por_tag'].items(), key=lambda x: x[1]['f1'])[:5]
    for tag, m in piores:
        print(f"  {tag:6s}: P={m['precision']:.3f} R={m['recall']:.3f} F1={m['f1']:.3f} (n={m['suporte']})")
    
    print("="*60 + "\n")