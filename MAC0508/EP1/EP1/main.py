from filtragem import *
from parteA import BigramModel, avaliar, metricas_por_palavra
from parteB import HMM, exportar_csv, importar_csv, avaliar_etiquetagem, imprimir_relatorio



def main():
    # ===== CARREGAMENTO =====
    df_train = ler_conllu('pt_porttinari-ud-train.conllu')
    df_dev = ler_conllu('pt_porttinari-ud-dev.conllu')
    df_test = ler_conllu('pt_porttinari-ud-test.conllu')

    df_train = filtrar_multiword(df_train)
    df_dev = filtrar_multiword(df_dev)
    df_test = filtrar_multiword(df_test)

    # ===== PARTE A =====
    print("\n=== PARTE A: Modelo de Bigramas ===")
    
    # Preparar dados
    sentencas_train_A = extrair_colunas_parteA(df_train)
    sentencas_dev_A = extrair_colunas_parteA(df_dev)
    sentencas_test_A = extrair_colunas_parteA(df_test)

    # Treinar modelo
    modelo_bigram = BigramModel(lambda_smooth=1.0)  # Laplace
    modelo_bigram.treinar(sentencas_train_A)

    # Processar desconhecidas do conjunto de validação
    modelo_bigram.processar_desconhecidas(sentencas_dev_A)

    # Prever
    previsoes_test_A = []
    for sentenca in sentencas_test_A:
        previsao_sentenca = modelo_bigram.prever_sentenca(sentenca)
        previsoes_test_A.append(previsao_sentenca)
    
    # Avaliar e mostrar a acurácia
    resultados_parte_A = avaliar(sentencas_test_A, previsoes_test_A)

    print("Accuracy:", resultados_parte_A["accuracy"])
    print("Total de palavras:", resultados_parte_A["total_palavras"])
    print("Palavras corretas:", resultados_parte_A["palavras_corretas"], "\n")

    # Código de teste:
    # df_metricas = metricas_por_palavra(sentencas_test_A, previsoes_test_A)

    # print("\nTop 20 palavras com melhor precision:")
    # print(df_metricas.sort_values("precision", ascending=False).head(20))

    # print("\nTop 20 palavras com melhor recall:")
    # print(df_metricas.sort_values("recall", ascending=False).head(20))

    # print("\nTop 20 palavras com melhor F1-score:")
    # print(df_metricas.sort_values("f1", ascending=False).head(20))

    # ===== PARTE B =====
    print("\n=== PARTE B: HMM com Viterbi ===")

    sentencas_train_B = extrair_colunas_parteB(df_train)
    sentencas_dev_B = extrair_colunas_parteB(df_dev)
    sentencas_test_B = extrair_colunas_parteB(df_test)

    # Treinar modelo HMM
    hmm = HMM(lambda_smooth=1.0, eps=1e-6)

    if input("Carregar modelo HMM de arquivo? (s/n): ").strip().lower() == 's':
        prefixo_arquivo = input("Digite o prefixo do arquivo (padrão: 'hmm_iter'): ").strip()
        if prefixo_arquivo == '':
            prefixo_arquivo = 'hmm_iter'
        importar_csv(hmm, prefixo_arquivo=prefixo_arquivo)
    
    if input("Treinar modelo HMM? (s/n): ").strip().lower() == 's':
        hmm.treinar(sentencas_train_B, eps=1e-3)

    if input("Exportar modelo HMM para arquivo? (s/n): ").strip().lower() == 's':
        exportar_csv(hmm, prefixo_arquivo="hmm_iter_updated")

    # Processar desconhecidas do conjunto de validação
    hmm.processar_desconhecidas(sentencas_dev_B)

    # Previsão usando Viterbi
    previsoes_test_B = hmm.etiquetar_corpus(sentencas_test_B, True)

    # Avaliar e mostrar a acurácia
    resultados = avaliar_etiquetagem(sentencas_test_B, previsoes_test_B)

    # Acessar métricas
    print(f"Acurácia: {resultados['accuracy']:.2%}")
    print(f"F1 Macro: {resultados['macro_avg']['f1']:.4f}")

    # Imprimir relatório detalhado por palavra
    if input("Imprimir relatório detalhado por palavra? (s/n): ").strip().lower() == 's':
        imprimir_relatorio(resultados)

if __name__ == "__main__":
    main()

