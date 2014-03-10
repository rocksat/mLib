
#ifndef EXT_EIGEN_EIGENSOLVER_H_
#define EXT_EIGEN_EIGENSOLVER_H_

namespace ml
{

template<class D> class LinearSolverEigen : public LinearSolver<D>
{
public:
	enum Method
	{
		LLT,
		LDLT,
		LU, //Inferior to LLT for symmetric problems
		QR, //Extremely slow
		ConjugateGradient_Diag,
		BiCGSTAB_Diag,
		BiCGSTAB_LUT,
		Profile,
	};

	LinearSolverEigen(Method method = ConjugateGradient_Diag)
	{
		m_method = method;
	}

	MathVector<D> solve(const SparseMatrix<D> &A, const MathVector<D> &b)
	{
		MLIB_ASSERT_STR(A.square() && b.size() == A.rows(), "invalid solve dimensions");
		Eigen::SparseMatrix<D> eigenMatrix;
		eigenutil::makeEigenMatrix(A, eigenMatrix);
		return solve(eigenMatrix, b);
	}

	MathVector<D> solve(const Eigen::SparseMatrix<D> &A, const MathVector<D> &b)
	{
		return solveUsingMethod(A, b, m_method);
	}

	MathVector<D> solveLeastSquares(const SparseMatrix<D> &A, const MathVector<D> &b)
	{
		//return solve(A.transpose() * A, b);

		Eigen::SparseMatrix<D> eigenMatrix;
		eigenutil::makeEigenMatrix(A, eigenMatrix);
		return solveLeastSquares(eigenMatrix, b);
	}

	MathVector<D> solveLeastSquares(const Eigen::SparseMatrix<D> &A, const MathVector<D> &b)
	{
		Console::log("Solving least-squares problem using QR");

		const Eigen::VectorXd bEigen = eigenutil::makeEigenVector(b);
		Eigen::SparseQR< Eigen::SparseMatrix<D>, Eigen::COLAMDOrdering<int> > factorization(A);
		Eigen::VectorXd x = factorization.solve(bEigen);

		return eigenutil::dumpEigenVector<D>(x);
	}

private:
	MathVector<D> solveUsingMethod(const Eigen::SparseMatrix<D> &A, const MathVector<D> &b, Method method)
	{
		ComponentTimer timer("Solving using method: " + getMethodName(method));
		
		const Eigen::VectorXd bEigen = eigenutil::makeEigenVector(b);
		Eigen::VectorXd x;

		if(method == LLT)
		{
			Eigen::SimplicialLLT< Eigen::SparseMatrix<double> > factorization(A);
			x = factorization.solve(bEigen);
		}
		else if(method == LDLT)
		{
			Eigen::SimplicialLDLT< Eigen::SparseMatrix<double> > factorization(A);
			x = factorization.solve(bEigen);
		}
		else if(method == LU)
		{
			Eigen::SparseLU< Eigen::SparseMatrix<double> > factorization(A);
			x = factorization.solve(bEigen);
		}
		else if(method == QR)
		{
			Eigen::SparseQR< Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int> > factorization(A);
			x = factorization.solve(bEigen);
		}
		else if(method == ConjugateGradient_Diag)
		{
			Eigen::ConjugateGradient< Eigen::SparseMatrix<double>, Eigen::Lower, Eigen::DiagonalPreconditioner<double > > solver;
			solver.setTolerance(1e-20);
			solver.compute(A);
			x = solver.solve(bEigen);
			//Console::log("Iterations: " + std::string(solver.iterations()));
			//Console::log("Error: " + std::string(solver.error()));
		}
		else if(method == BiCGSTAB_Diag)
		{
			Eigen::BiCGSTAB< Eigen::SparseMatrix<double>, Eigen::DiagonalPreconditioner<double > > solver;
			solver.setTolerance(1e-10);
			solver.compute(A);
			x = solver.solve(bEigen);
			//Console::log("Iterations: " + std::string(solver.iterations()));
			//Console::log("Error: " + std::string(solver.error()));
		}
		else if(method == BiCGSTAB_LUT)
		{
			Eigen::BiCGSTAB< Eigen::SparseMatrix<double>, Eigen::IncompleteLUT<double > > solver;
			solver.setTolerance(1e-10);
			solver.compute(A);
			x = solver.solve(bEigen);
			//Console::log("Iterations: " + std::string(solver.iterations()));
			//Console::log("Error: " + std::string(solver.error()));
		}
		else if(method == Profile)
		{
			Console::log("Profiling all eigen linear solvers");
			const int methodCount = (int)Profile;
			std::vector< MathVector<D> > results(methodCount);
			for(int methodIndex = 0; methodIndex < methodCount; methodIndex++)
			{
				results[methodIndex] = solveUsingMethod(A, b, (Method)methodIndex);
				if(methodIndex != 0)
				{
					double maxDeviation = 0.0;
					for(UINT variableIndex = 0; variableIndex < b.size(); variableIndex++)
						maxDeviation = std::max(maxDeviation, fabs(results[methodIndex][variableIndex] - results[0][variableIndex]));
					Console::log("Max deviation from LLT: " + std::to_string(maxDeviation));
				}
			}
			return results[0];
		}
		else
		{
			MLIB_ERROR("Unknown method");
		}

		return eigenutil::dumpEigenVector<D>(x);
	}

	static std::string getMethodName(Method m)
	{
		switch(m)
		{
		case LLT: return "LLT";
		case LDLT: return "LDLT";
		case LU: return "LU";
		case QR: return "QR";
		case ConjugateGradient_Diag: return "ConjugateGradient_Diag";
		case BiCGSTAB_Diag: return "BiCGSTAB_Diag";
		case BiCGSTAB_LUT: return "BiCGSTAB_LUT";
		case Profile: return "Profile";
		default: return "Unknown";
		}
	}

	Method m_method;
};

}  // namespace ml

#endif  // EXT_EIGEN_EIGENSOLVER_H_