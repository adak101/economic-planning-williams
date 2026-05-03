// Zadanie 12.9
// Model LP
// Uruchomienie: ./economic_planning [1|2|3]

#include <ilcplex/ilocplex.h>
#include <iostream>
#include <iomanip>

ILOSTLBEGIN

int main(int argc, char* argv[])
{
    int objective = 1;
    if (argc > 1) objective = atoi(argv[1]);

    IloEnv env;
    try {
        IloModel model(env);

        // DANE WEJSCIOWE
        const int NI = 3;  // 0=coal, 1=steel, 2=transport
        const char* names[] = {"Coal", "Steel", "Transport"};

        // Tabela 12.1 - naklady na produkcje
        double a[4][3] = {
            {0.1, 0.5, 0.4},  // coal
            {0.1, 0.1, 0.2},  // steel
            {0.2, 0.1, 0.2},  // transport
            {0.6, 0.3, 0.2}   // manpower
        };

        // Tabela 12.2 - naklady na rozbudowe
        double b[4][3] = {
            {0.0, 0.7, 0.9},  // coal
            {0.1, 0.1, 0.2},  // steel
            {0.2, 0.1, 0.2},  // transport
            {0.4, 0.2, 0.1}   // manpower
        };

        // Tabela 12.3 - dane startowe
        double s0[3]   = {150, 80, 100};
        double cap0[3] = {300, 350, 280};
        double manpowerLimit = 470;

        // Popyt: cele 1 i 3 -> 60/60/30, cel 2 -> 0
        double demand[3] = {0, 0, 0};
        if (objective == 1 || objective == 3) {
            demand[0] = 60;  demand[1] = 60;  demand[2] = 30;
        }

        // ZMIENNE DECYZYJNE
        // naklady w roku t -> produkcja w t+1, rozbudowa w t+2

        // x[i][t] - produkcja branzy i w roku t
        IloArray<IloNumVarArray> x(env, NI);
        for (int i = 0; i < NI; i++) {
            x[i] = IloNumVarArray(env, 6, 0.0, IloInfinity);
            x[i][0].setUB(0.0);
        }

        // y[i][t] - rozbudowa branzy i, zaczeta w roku t
        IloArray<IloNumVarArray> y(env, NI);
        for (int i = 0; i < NI; i++) {
            y[i] = IloNumVarArray(env, 4, 0.0, IloInfinity);
        }

        // s[i][t] - zapas dobra i w roku t
        IloArray<IloNumVarArray> s(env, NI);
        for (int i = 0; i < NI; i++) {
            s[i] = IloNumVarArray(env, 7, 0.0, IloInfinity);
            s[i][0].setBounds(s0[i], s0[i]);
        }

        // 1) BILANS MATERIALOWY
        // Rok 0
        for (int j = 0; j < NI; j++) {
            IloExpr expr(env);
            expr += s[j][0];
            for (int i = 0; i < NI; i++) {
                expr -= a[j][i] * x[i][1];
                expr -= b[j][i] * y[i][0];
            }
            expr -= s[j][1];
            model.add(expr == 0);
            expr.end();
        }

        // Lata 1..4
        for (int t = 1; t <= 4; t++) {
            for (int j = 0; j < NI; j++) {
                IloExpr expr(env);
                expr += s[j][t] + x[j][t];
                for (int i = 0; i < NI; i++) {
                    expr -= a[j][i] * x[i][t + 1];
                }
                if (t <= 3) {
                    for (int i = 0; i < NI; i++) {
                        expr -= b[j][i] * y[i][t];
                    }
                }
                expr -= demand[j];
                expr -= s[j][t + 1];
                model.add(expr == 0);
                expr.end();
            }
        }

        // Rok 5
        for (int j = 0; j < NI; j++) {
            IloExpr expr(env);
            expr += s[j][5] + x[j][5];
            expr -= demand[j];
            expr -= s[j][6];
            model.add(expr == 0);
            expr.end();
        }

        // 2) LIMITY ZDOLNOSCI PRODUKCYJNEJ
        for (int i = 0; i < NI; i++) {
            for (int t = 1; t <= 5; t++) {
                IloExpr cap(env);
                cap += cap0[i];
                for (int tau = 0; tau <= t - 2 && tau <= 3; tau++) {
                    cap += y[i][tau];
                }
                model.add(x[i][t] <= cap);
                cap.end();
            }
        }

        // 3) LIMIT MANPOWER (wylaczone dla celu 3)
        if (objective != 3) {
            for (int t = 0; t <= 4; t++) {
                IloExpr man(env);
                for (int i = 0; i < NI; i++) {
                    man += a[3][i] * x[i][t + 1];
                }
                if (t <= 3) {
                    for (int i = 0; i < NI; i++) {
                        man += b[3][i] * y[i][t];
                    }
                }
                model.add(man <= manpowerLimit);
                man.end();
            }
        }

        // FUNKCJA CELU
        IloExpr obj(env);

        if (objective == 1) {
            // Max zdolnosc produkcyjna w roku 5
            for (int i = 0; i < NI; i++) {
                obj += cap0[i];
                for (int tau = 0; tau <= 3; tau++) {
                    obj += y[i][tau];
                }
            }
        }
        else if (objective == 2) {
            // Max produkcja w latach 4 i 5
            for (int i = 0; i < NI; i++) {
                obj += x[i][4] + x[i][5];
            }
        }
        else if (objective == 3) {
            // Max zatrudnienie
            for (int t = 0; t <= 4; t++) {
                for (int i = 0; i < NI; i++) {
                    obj += a[3][i] * x[i][t + 1];
                }
                if (t <= 3) {
                    for (int i = 0; i < NI; i++) {
                        obj += b[3][i] * y[i][t];
                    }
                }
            }
        }

        model.add(IloMaximize(env, obj));
        obj.end();

        // ROZWIAZANIE
        IloCplex cplex(model);

        if (!cplex.solve()) {
            cout << "Brak rozwiazania. Status: " << cplex.getStatus() << endl;
            env.end();
            return 1;
        }

        cout << fixed << setprecision(2);
        cout << "\nCel " << objective << " - wartosc: " << cplex.getObjValue() << "\n\n";

        // Produkcja
        cout << "Produkcja x[i][t]:\n";
        for (int i = 0; i < NI; i++) {
            cout << names[i] << ":\t";
            for (int t = 1; t <= 5; t++) cout << cplex.getValue(x[i][t]) << "\t";
            cout << endl;
        }

        // Rozbudowa
        cout << "\nRozbudowa y[i][t]:\n";
        for (int i = 0; i < NI; i++) {
            cout << names[i] << ":\t";
            for (int t = 0; t <= 3; t++) cout << cplex.getValue(y[i][t]) << "\t";
            cout << endl;
        }

        // Zapasy
        cout << "\nZapasy s[i][t]:\n";
        for (int i = 0; i < NI; i++) {
            cout << names[i] << ":\t";
            for (int t = 0; t <= 6; t++) cout << cplex.getValue(s[i][t]) << "\t";
            cout << endl;
        }

        // Manpower
        cout << "\nManpower (zuzycie / limit):\n";
        for (int t = 0; t <= 4; t++) {
            double man = 0;
            for (int i = 0; i < NI; i++) man += a[3][i] * cplex.getValue(x[i][t + 1]);
            if (t <= 3) {
                for (int i = 0; i < NI; i++) man += b[3][i] * cplex.getValue(y[i][t]);
            }
            cout << "Rok " << t << ": " << man << " / " << manpowerLimit << endl;
        }
    }
    catch (IloException& e) {
        cerr << "Blad CPLEX: " << e << endl;
    }

    env.end();
    return 0;
}