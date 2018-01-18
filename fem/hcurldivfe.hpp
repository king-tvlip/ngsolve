#ifndef FILE_HCURLDIVFE
#define FILE_HCURLDIVFE

/*********************************************************************/
/* File:   hcurldivfe.hpp                                            */
/* Author: Philip Lederer                                            */
/* Date:   2017/2018                                                 */
/*********************************************************************/


namespace ngfem
{

  template <int DIM>
  class HCurlDivFiniteElement : public FiniteElement
  {
  public:
    using FiniteElement::FiniteElement;
    using FiniteElement::ndof;
    using FiniteElement::order;

    // old style
    virtual void CalcShape (const IntegrationPoint & ip, 
                            BareSliceMatrix<double> shape) const = 0;

    virtual void CalcDivShape (const IntegrationPoint & ip, 
                               BareSliceMatrix<double> divshape) const = 0;
    
    virtual void CalcMappedShape (const MappedIntegrationPoint<DIM,DIM> & mip,
      BareSliceMatrix<double> shape) const = 0;    

    virtual void CalcMappedDivShape (const MappedIntegrationPoint<DIM,DIM> & mip,
      BareSliceMatrix<double> shape) const = 0;

  };
  
  template <int D,typename VEC,typename MAT>
  void VecToMat(const VEC & vec,MAT & mat)
  {
    switch(D)
    {
    case 2:
      mat(0) = vec(0);
      mat(1) = vec(1);
      mat(2) = vec(2);
      mat(3) = vec(3);
      break;
    case 3:
      mat(0) = vec(0);
      mat(1) = vec(1);
      mat(2) = vec(2);
      mat(3) = vec(3);
      mat(4) = vec(4);
      mat(5) = vec(5);
      mat(6) = vec(6);
      mat(7) = vec(7);
      mat(8) = vec(8);
      break;
    }

  }

  
  template <ELEMENT_TYPE ET> class HCurlDivFE;

  
  template <ELEMENT_TYPE ET>
  class T_HCurlDivFE : public HCurlDivFiniteElement<ET_trait<ET>::DIM>,
    public VertexOrientedFE<ET>
  {
  protected:
    enum { DIM = ET_trait<ET>::DIM };
    enum { DIM_STRESS = DIM*DIM };
    
    using VertexOrientedFE<ET>::vnums;
    using HCurlDivFiniteElement<ET_trait<ET>::DIM>::ndof;
    using HCurlDivFiniteElement<ET_trait<ET>::DIM>::order;

    int order_facet[ET_trait<ET>::N_FACET];
    int order_inner;

    // additional curl-div free bubbles
    bool plus;

  public:
    using VertexOrientedFE<ET>::SetVertexNumbers;
    
    T_HCurlDivFE (int aorder, bool _plus = false)
      : plus(_plus)
    {
      order = aorder;
      for (auto & of : order_facet) of = aorder;
      order_inner = aorder;
    }
    
    virtual ELEMENT_TYPE ElementType() const { return ET; }
    const HCurlDivFE<ET> * Cast() const { return static_cast<const HCurlDivFE<ET>*> (this); } 
    
    INLINE void SetOrderFacet (int nr, int order) { order_facet[nr] = order; }
    INLINE void SetOrderInner (int order) { order_inner = order; }

    virtual void ComputeNDof()
    {
      cout << "Error, T_HCurlDivFE<ET>:: ComputeNDof not available, only for ET == TRIG" << endl;
    }

    virtual void CalcShape (const IntegrationPoint & ip, 
                            BareSliceMatrix<double> shape) const
    {
      Vec<DIM, AutoDiffDiff<DIM>> adp;
      for ( int i=0; i<DIM; i++)
      {
        adp(i) = AutoDiffDiff<DIM>(ip(i),i);
      }

      Cast() -> T_CalcShape (TIP<DIM, AutoDiffDiff<DIM>> (adp), SBLambda([&] (int nr, auto val)
                                          {
                                            shape.Row(nr).AddSize(DIM_STRESS) = val.Shape();
                                          }));
    }

    virtual void CalcDivShape (const IntegrationPoint & ip,
                               BareSliceMatrix<double> shape) const
    {
      Vec<DIM, AutoDiffDiff<DIM>> adp;
      for ( int i=0; i<DIM; i++)
      {
        adp[i] = AutoDiffDiff<DIM>(ip(i),i);
      }
      
      Cast() -> T_CalcShape (TIP<DIM, AutoDiffDiff<DIM>> (adp), SBLambda([&] (int nr, auto val)
                                          {
                                            shape.Row(nr).AddSize(DIM) = val.DivShape();
                                          }));
    }


    virtual void CalcMappedShape (const MappedIntegrationPoint<DIM,DIM> & mip,
                            BareSliceMatrix<double> shape) const
    {
      Vec<DIM, AutoDiff<DIM>> adp = mip;
      Vec<DIM, AutoDiffDiff<DIM>> addp;
      for (int i=0; i<DIM; i++)
      {
        addp[i] = adp[i].Value();
        addp[i].LoadGradient(&adp[i].DValue(0));
      }
      Cast() -> T_CalcShape (TIP<DIM,AutoDiffDiff<DIM>> (addp),SBLambda([&](int nr,auto val)
      {
        Vec<DIM_STRESS> vecshape = val.Shape();
        BareVector<double> matshape = shape.Row(nr);
        VecToMat<DIM> (vecshape, matshape);
      }));
    }


    virtual void CalcMappedDivShape (const MappedIntegrationPoint<DIM,DIM> & mip,
                            BareSliceMatrix<double> shape) const
    {
      Vec<DIM, AutoDiff<DIM>> adp = mip;
      Vec<DIM, AutoDiffDiff<DIM>> addp;
      for (int i=0; i<DIM; i++)
      {
        addp[i] = adp[i].Value();
        addp[i].LoadGradient(&adp[i].DValue(0));
      }

      if(!mip.GetTransformation().IsCurvedElement()) // non-curved element
      {
        Cast() -> T_CalcShape (TIP<DIM,AutoDiffDiff<DIM>> (addp),SBLambda([&](int nr,auto val)
        {
          shape.Row(nr).AddSize(DIM) = val.DivShape();
        }));
      }
      else // curved element
      {
	throw Exception ("not implemented yet!");
        /*Mat<DIM> jac = mip.GetJacobian();
        Mat<DIM> inv_jac = mip.GetJacobianInverse();
        Mat<DIM> hesse[3],finvT_h_tilde_finv[3];
        mip.CalcHesse (hesse[0],hesse[1],hesse[2]);

        Mat<DIM,DIM,AutoDiff<DIM> > f_tilde;
        for(int i = 0; i < DIM; i++)
        {
          for(int j = 0; j < DIM; j++)
          {
            f_tilde(i,j).Value() = jac(i,j);
            for(int k = 0; k < DIM; k++)
              f_tilde(i,j).DValue(k) = hesse[i](j,k);
          }
        }

        AutoDiff<DIM> ad_det = Det (f_tilde);
        AutoDiff<DIM> iad_det = 1.0 / ad_det;
        f_tilde *= iad_det;

        for(int i=0; i<DIM; i++)
        {
          finvT_h_tilde_finv[i] = 0;
          for(int alpha=0; alpha<DIM; alpha++)
            for(int beta=0; beta<DIM; beta++)
              for(int gamma=0; gamma<DIM; gamma++)
                for(int delta=0; delta<DIM; delta++)
                  finvT_h_tilde_finv[i](alpha,beta) += inv_jac(gamma,alpha)*f_tilde(i,gamma).DValue(delta)*inv_jac(delta,beta);
        }

        Cast() -> T_CalcShape (TIP<DIM,AutoDiffDiff<DIM>> (addp),SBLambda([&](int nr,auto val)
                                  {
                                    shape.Row(nr).AddSize(DIM) = val.DivShape();
                                    BareVector<double> divshape = shape.Row(nr);
                                    Vec<DIM_STRESS> vecshape = val.Shape();
                                    Vec<DIM*DIM> matshape;
                                    VecToSymMat<DIM> (vecshape, matshape);

                                    for(int k=0; k<DIM; k++)
                                    {
                                      for(int j=0; j<DIM*DIM; j++)
                                      {
                                        divshape(k) += mip.GetJacobiDet() * finvT_h_tilde_finv[k](j) * matshape(j);
                                      }
                                    }
                                    
                                  }));
	*/
      }
    }

  };

  /* ############### edge basis functions - div-free ############### */
  /* sigma(grad v) = Curl(grad v), where Curl is the 1D to 2D curl operator */
  
  class Sigma_gradv
  {
    AutoDiffDiff<2> v;
  public:
    Sigma_gradv  (AutoDiffDiff<2> av) : v(av){ ; }
    
    Vec<4> Shape() {
      return Vec<4> (-v.DDValue(0,1), v.DDValue(0,0),
		     -v.DDValue(1,1),v.DDValue(0,1)
		     );
    }

    Vec<2> DivShape()
    {      
      return Vec<2> (0,0);     
    }
  };

  
  /* ############### Type 1 - inner basis functions - div-free ############### */
  /* sigma(grad(u)*v) = Curl(grad(u)) * v + grad(u) * Curl(v) */  

  class Sigma_gradu_v
  {
    AutoDiffDiff<2> u,v;
  public:
    Sigma_gradu_v  (AutoDiffDiff<2> au, AutoDiffDiff<2> av) : u(au), v(av){ ; }

    Vec<4> Shape() {
      return Vec<4> (-u.DDValue(1,0) * v.Value()  -  v.DValue(1)*u.DValue(0),
		     u.DDValue(0,0) * v.Value() + v.DValue(0)*u.DValue(0),
		     -u.DDValue(1,1) * v.Value() - v.DValue(1)*u.DValue(1),
		     u.DDValue(0,1) * v.Value() +  v.DValue(0)*u.DValue(1)
		     );
    }

    Vec<2> DivShape()
    {     
      return Vec<2> (0,0);
    }
  }; 

  /* ############### Type 2 - inner basis functions - NOT div-free ############### */
  /* sigma(grad(u)*v) = Curl(grad(u)) * v - grad(u) * Curl(v) */ 
  class Curlgraduv_graducurlv
  {
    AutoDiffDiff<2> u,v;
  public:
    Curlgraduv_graducurlv  (AutoDiffDiff<2> au, AutoDiffDiff<2> av) : u(au), v(av){ ; }

    Vec<4> Shape() {
      return Vec<4> (-u.DDValue(1,0) * v.Value()  +  v.DValue(1)*u.DValue(0),
		     u.DDValue(0,0) * v.Value() - v.DValue(0)*u.DValue(0),
		     -u.DDValue(1,1) * v.Value() + v.DValue(1)*u.DValue(1),
		     u.DDValue(0,1) * v.Value() -  v.DValue(0)*u.DValue(1)
		     );
    }

    Vec<2> DivShape()
    {
      double uxx = u.DDValue(0,0), uxy = u.DDValue(0,1), uyy = u.DDValue(1,1);
      double vx = v.DValue(0), vy = v.DValue(1);
      
      return -2.0 * Vec<2> (- uxx * vy + uxy * vx, - uxy * vy + uyy * vx);
    }
  };   

  /* ############### Type 3 - inner basis functions - div-free ############### */
  /*  Curl( [grad(l1) l2 - l1 grad(l2)] * v ) */  

  class T_type4
  {
    AutoDiffDiff<2> l1,l2,v;
  public:
    T_type4  (AutoDiffDiff<2> lam1, AutoDiffDiff<2> lam2, AutoDiffDiff<2> av) : l1(lam1), l2(lam2), v(av){ ; }

    Vec<4> Shape() {
      double lam1x = l1.DValue(0), lam1y = l1.DValue(1), lam1xx = l1.DDValue(0,0), lam1xy = l1.DDValue(1,0), lam1yx = l1.DDValue(0,1), lam1yy = l1.DDValue(1,1);
      double lam2x = l2.DValue(0), lam2y = l2.DValue(1), lam2xx = l2.DDValue(0,0), lam2xy = l2.DDValue(1,0), lam2yx = l2.DDValue(0,1), lam2yy = l2.DDValue(1,1);

      double vx = v.DValue(0), vy = v.DValue(1);
                 
      return Vec<4> (v.Value() * (-lam1yx * l2.Value() - lam1x * lam2y + lam2yx * l1.Value() + lam2x * lam1y) - (lam1x*l2.Value() - lam2x*l1.Value()) * vy,
		     v.Value() * ( lam1xx * l2.Value() + lam1x * lam2x - lam2xx * l1.Value() - lam2x * lam1x) + (lam1x*l2.Value() - lam2x*l1.Value()) * vx,
		     v.Value() * (-lam1yy * l2.Value() - lam1y * lam2y + lam2yy * l1.Value() + lam2y * lam1y) - (lam1y*l2.Value() - lam2y*l1.Value()) * vy,
		     v.Value() * ( lam1xy * l2.Value() + lam1y * lam2x - lam2xy * l1.Value() - lam2y * lam1x) + (lam1y*l2.Value() - lam2y*l1.Value()) * vx
		     );     
    }

    Vec<2> DivShape()
    {     
      return Vec<2> (0,0);
    }
  };  
  
  /* ############### Special functions for curld-div bubbles ############### */
  /* sigma(u * grad v) = Curl(u * grad v) - tr(Curl(u * grad v)) * I  */  
  
  class T_sigma_u_grad_v
  {
    AutoDiffDiff<2> u,v;
  public:
    T_sigma_u_grad_v  (AutoDiffDiff<2> au, AutoDiffDiff<2> av) : u(au), v(av){ ; }

    Vec<4> Shape() {
      return Vec<4> (- u.Value()*v.DDValue(0,1) - 0.5*(u.DValue(1)*v.DValue(0) + u.DValue(0)*v.DValue(1)),
		     u.DValue(0)*v.DValue(0) + u.Value()*v.DDValue(0,0),
		     -u.DValue(1)*v.DValue(1) - u.Value()*v.DDValue(1,1),
		     u.Value()*v.DDValue(1,0) + 0.5*(u.DValue(1)*v.DValue(0) + u.DValue(0)*v.DValue(1))
		     );
    }

    Vec<2> DivShape()
    {
      double uxx = u.DDValue(0,0), uyy = u.DDValue(1,1), uxy = u.DDValue(0,1);
      double ux = u.DValue(0), uy = u.DValue(1);
      double vxx = v.DDValue(0,0), vyy = v.DDValue(1,1), vxy = v.DDValue(0,1);
      double vx = v.DValue(0), vy = v.DValue(1);
      
      return -0.5 * Vec<2> (-vx*uxy-uy*vxx+vxy*ux+vy*uxx,
                            vyy*ux+vy*uxy-vxy*uy-vx*uyy);

    }
  };

  

  /* Edge basis functions which are normal-tangential continuous */
  /* calculates [(grad l1) o-times (rot-grad l2) ] * scaledlegendre */
  /* DivShape assumes that phi_12 = [(grad l1) o-times (rot-grad l2) ] is constant!!! */
  /* This is the old implementation! */
  class T_Dl2xRotDl1_v
  {
    AutoDiffDiff<2> l1,l2,v;
  public:
    T_Dl2xRotDl1_v  (AutoDiffDiff<2> lam1, AutoDiffDiff<2> lam2, AutoDiffDiff<2> av) : l1(lam1), l2(lam2), v(av) { ; }

    Vec<4> Shape() {
      return Vec<4> (-v.Value()*(l1.DValue(0)*l2.DValue(1)),
		     v.Value()*(l1.DValue(0)*l2.DValue(0)),
		     -v.Value()*(l1.DValue(1)*l2.DValue(1)),
		     v.Value()*(l1.DValue(1)*l2.DValue(0))
		     );
    }

    Vec<2> DivShape()
    {
      double vx = v.DValue(0), vy = v.DValue(1);

      double lam1x = l1.DValue(0);
      double lam1y = l1.DValue(1);      
      
      double lam2x = l2.DValue(0);
      double lam2y = l2.DValue(1);
      
      return Vec<2> (  - vx *lam1x*lam2y + vy*lam1x*lam2x, -vx *lam1y*lam2y + vy*lam1y*lam2x) ;      
    }

  };

  
  /* Face basis functions which are normal-tangential continuous */
  /* calculates [(grad l1) o-times (grad l2 x grad l3)] * legendre */
  /* DivShape assumes that phi_12 =  [(grad l1) o-times (grad l2 x grad l3)] is constant!!! */
  class T_Dl1_o_Dl2xDl3_v
  {
    AutoDiffDiff<3> l1,l2,l3,v;
  public:
    T_Dl1_o_Dl2xDl3_v  (AutoDiffDiff<3> lam1, AutoDiffDiff<3> lam2, AutoDiffDiff<3> lam3, AutoDiffDiff<3> av) : l1(lam1), l2(lam2), l3(lam3), v(av) { ; }
    
    Vec<9> Shape() {

      double cross1 = l2.DValue(1)*l3.DValue(2) - l2.DValue(2)*l3.DValue(1);
      double cross2 = -(l2.DValue(0)*l3.DValue(2) - l2.DValue(2)*l3.DValue(0));
      double cross3 = l2.DValue(0)*l3.DValue(1) - l2.DValue(1)*l3.DValue(0);

      Vec<9> sigmaref;
      
      for (int i=0; i<3; i++)
	{
	  sigmaref(i*3)= v.Value() * l1.DValue(i) * cross1;
	  sigmaref(i*3+1)= v.Value() * l1.DValue(i) * cross2;
	  sigmaref(i*3+2)= v.Value() * l1.DValue(i) * cross3;
	}
      return sigmaref;  

    }

    Vec<3> DivShape()
    {
      double vx = v.DValue(0), vy = v.DValue(1), vz = v.DValue(2);

      double cross1 = l2.DValue(1)*l3.DValue(2) - l2.DValue(2)*l3.DValue(1);
      double cross2 = -(l2.DValue(0)*l3.DValue(2) - l2.DValue(2)*l3.DValue(0));
      double cross3 = l2.DValue(0)*l3.DValue(1) - l2.DValue(1)*l3.DValue(0);

      return Vec<3> (vx * l1.DValue(0) * cross1 + vy * l1.DValue(0) * cross2 + vz * l1.DValue(0) * cross3,
		     vx * l1.DValue(1) * cross1 + vy * l1.DValue(1) * cross2 + vz * l1.DValue(1) * cross3,
		     vx * l1.DValue(2) * cross1 + vy * l1.DValue(2) * cross2 + vz * l1.DValue(2) * cross3
		     );
    }

  };

  
  /* Identity = Inner bubble function (normatl-tangential component is zero) */
  /* calculates I * legendre */
  template <int D>
  class T_Id_v
  {
    AutoDiffDiff<D> v;
  public:
    T_Id_v  (AutoDiffDiff<D> av) : v(av) { ; }
    
    Vec<D*D> Shape() {
      Vec<D*D> Id_v=0;

      for (int i=0; i<D; i++)
	Id_v(i*(D+1))= v.Value();      
      return Id_v;
    }

    Vec<D> DivShape()
    {
      Vec<D> div_Id_v=0;
      for (int i=0; i<D; i++)
	div_Id_v(i) = v.DValue(i);
      return div_Id_v;
    }

  };
  
  template <> class HCurlDivFE<ET_TRIG> : public T_HCurlDivFE<ET_TRIG> 
  {
    
  public:
    using T_HCurlDivFE<ET_TRIG> :: T_HCurlDivFE;

    virtual void ComputeNDof()
    {     
      order = 0;
      ndof = 0;
      for (int i=0; i<3; i++)
      {
        ndof += order_facet[i]+1;
        order = max2(order, order_facet[i]);
      }      
      int ninner = order_inner +1 + 2 * ((order_inner +1) * (order_inner)); 
      order = max2(order, order_inner);
      if (plus)
      {
	//throw Exception(" please update this first - ComputeNdof in HCurlDiv<ET_TRIG>");
        order ++;
        ninner += 2*(order_inner); 
      }
      ndof += ninner;      
    }
    
   template <typename Tx, typename TFA> 
    void T_CalcShape (TIP<2,Tx> ip/*AutoDiffDiff<2> hx[2]*/, TFA & shape) const
    {
      auto x = ip.x, y = ip.y;
      AutoDiffDiff<2> ddlami[3] ={ x, y, 1-x-y };
      
      int ii = 0;
      
      int maxorder_facet =
        max2(order_facet[0],max2(order_facet[1],order_facet[2]));

      const EDGE * edges = ElementTopology::GetEdges(ET_TRIG);

      ArrayMem<AutoDiffDiff<2>,20> ha(maxorder_facet+1);
      ArrayMem<AutoDiffDiff<2>,20> v(order_inner+1), dubb(order_inner*(order_inner+1)/2), u(order_inner + 3);

      /* Edge based basis functions for tangential-normal continuity */
      for (int i = 0; i < 3; i++)
        {
          int es = edges[i][0], ee = edges[i][1];
	  	  
          if (vnums[es] > vnums[ee]) swap (es,ee);
	  	  
          AutoDiffDiff<2> ls = ddlami[es], le = ddlami[ee];
	  
	  ScaledLegendrePolynomial(maxorder_facet,le-ls, le+ls,ha);
          
          for (int l = 0; l <= order_facet[i]; l++)	    
	    shape[ii++] = Sigma_gradv(le*ls*ha[l]);            
        }
      
      int es = 0; int ee = 1; int et = 2;
      AutoDiffDiff<2> ls = ddlami[es];
      AutoDiffDiff<2> le = ddlami[ee];
      AutoDiffDiff<2> lt = ddlami[et];
      
      int oi=order_inner;

      //ScaledIntegratedLegendrePolynomial(oi+3,le-lt,le+lt,u);
      IntegratedLegendreMonomialExt::CalcTrigExt(oi+3,le-lt,1-le-lt,u);
      LegendrePolynomial::EvalMult(oi+1, 2*ls-1, ls, v);
      
      for(int i = 0; i <= oi-1; i++)
      {
        for(int j = 0; j+i <= oi-1; j++)
        {
	  shape[ii++] = Sigma_gradu_v(u[i],v[j]);
	  shape[ii++] = Curlgraduv_graducurlv(u[i],v[j]);	  	  
        }	
      }

      //ScaledIntegratedLegendrePolynomial(oi+3,le-ls,le+ls,u);
      IntegratedLegendreMonomialExt::CalcTrigExt(oi+3,le-ls,1-le-ls,u);
      LegendrePolynomial::EvalMult(oi+1, 2*lt-1, lt, v);
      
      for(int i = 0; i <= oi-1; i++)
      {
        for(int j = 0; j+i <= oi-1; j++)
        {
	  shape[ii++] = Sigma_gradu_v(u[i],v[j]);
	  shape[ii++] = Curlgraduv_graducurlv(u[i],v[j]);	 
        }	
      }
      
      LegendrePolynomial::Eval(oi, 2*lt-1, v);
      for (int i = 0; i <= oi; i++)
        shape[ii++] = T_type4(le, ls, v[i]);
      
      
      /*
      //old basis   
      LegendrePolynomial::Eval(oi, 2*lt-1, v); 
      DubinerBasis3::Eval (oi-1, ls, le, dubb); 
      //Type one
      for (int i = 0; i < oi+1; i++)
	{	  
	shape[ii++] = T_Id_v<2>(v[i]);
	}      
      
      for(int i = 0; i < (oi+1)*oi/2; i++)
      {	
      	shape[ii++] = T_Dl2xRotDl1_v(ls,lt,le*dubb[i]);
      	shape[ii++] = T_Dl2xRotDl1_v(lt,ls,le*dubb[i]);
      	shape[ii++] = T_Dl2xRotDl1_v(le,lt,ls*dubb[i]);
      	shape[ii++] = T_Dl2xRotDl1_v(ls,le,lt*dubb[i]);	  
      }
      */
      
      IntegratedLegendreMonomialExt::CalcTrigExt(oi+3,le-ls,1-le-ls,u);
      //ScaledIntegratedLegendrePolynomial(oi+3,le-ls,le+ls,u);
      LegendrePolynomial::EvalMult(oi+1, 2*lt-1, lt, v);
      
      if (plus)
        for (int i = 0; i <= oi-1; i++)
          {
	    throw Exception("not working yet!");
	    AutoDiffDiff<2> bubble = u[i]*v[oi-1-i];
	    
	    shape[ii++] = T_sigma_u_grad_v(bubble,x);
	    shape[ii++] = T_sigma_u_grad_v(bubble,y);
          }      
    };
  }; 
 
     template <> class HCurlDivFE<ET_TET> : public T_HCurlDivFE<ET_TET> 
  {
    
  public:
    using T_HCurlDivFE<ET_TET> :: T_HCurlDivFE;

    virtual void ComputeNDof()
    {     
      order = 0;
      ndof = 0;
      for (int i=0; i<4; i++)
      {
        ndof += (order_facet[i]+1)*(order_facet[i]+2);
        order = max2(order, order_facet[i]);
      }
      //first type + second type
      int ninner = (order_inner + 1)*(order_inner + 2)*(order_inner + 3)/6.0 + 8.0/6.0* ((order_inner +2) * (order_inner +1) * (order_inner));
      
      order = max2(order, order_inner);
      //if (plus)
      //{	
      //  order ++;
      //  ninner += 2*order_inner; 
      //}
      ndof += ninner;     
    }
    
   template <typename Tx, typename TFA> 
    void T_CalcShape (TIP<3,Tx> ip/*AutoDiffDiff<2> hx[2]*/, TFA & shape) const
    {
      auto x = ip.x, y = ip.y, z = ip.z ;
      AutoDiffDiff<3> ddlami[4] ={ x, y, z, 1-x-y-z };
      
      int ii = 0;
      
      int maxorder_facet =
        max2(order_facet[0],max2(order_facet[1],order_facet[2]));


       const FACE * faces = ElementTopology::GetFaces(ET_TET);

       
       ArrayMem<AutoDiffDiff<3>,20> ha((maxorder_facet+1)*(maxorder_facet+2)/2.0); 
       ArrayMem<AutoDiffDiff<3>,20> v((order_inner + 1)*(order_inner + 2)*(order_inner + 3)/6.0);
       ArrayMem<AutoDiffDiff<3>,20> dubb(order_inner*(order_inner+1)*(order_inner+2)/6.0);
       
      /* Edge based basis functions for tangential-normal continuity */
      for(int fa = 0; fa < 4; fa++)
        {
	  int fav[3] = {faces[fa][0], faces[fa][1], faces[fa][2]};
	  
	  int p = order_facet[fa];
	  //Sort vertices  first edge op minimal vertex
	  if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0], fav[1]);
	  if(vnums[fav[1]] > vnums[fav[2]]) swap(fav[1], fav[2]);
	  if(vnums[fav[0]] > vnums[fav[1]]) swap(fav[0], fav[1]);


          AutoDiffDiff<3> ls = ddlami[fav[0]], le = ddlami[fav[1]], lt = ddlami[fav[2]];
	  
	  DubinerBasis3::Eval (maxorder_facet, ls, le, ha);

          for (int l = 0; l < (order_facet[fa]+1)*(order_facet[fa]+2)/2.0; l++)
	    {	      
	      shape[ii++] =  T_Dl1_o_Dl2xDl3_v(le,ls,lt,ha[l]);	      
	      shape[ii++] =  T_Dl1_o_Dl2xDl3_v(ls,lt,le,ha[l]);
	    }
        }

      int oi=order_inner;
      
      int es = 0; int ee = 1; int et = 2; int eo = 3 ;
      AutoDiffDiff<3> ls = ddlami[es]; 
      AutoDiffDiff<3> le = ddlami[ee];
      AutoDiffDiff<3> lt = ddlami[et];
      AutoDiffDiff<3> lo = ddlami[eo];

      LegendrePolynomial leg;
      JacobiPolynomialAlpha jac1(1);

      //############ type 1 ############
      
      leg.EvalScaled1Assign 
	(oi, lt-lo, lt+lo,
	 SBLambda ([&](size_t k, Tx polz) LAMBDA_INLINE
		   {
		     JacobiPolynomialAlpha jac2(2*k+2);
 
		     jac1.EvalScaledMult1Assign
		       (oi-k, le-lt-lo, 1-ls, polz, 
			SBLambda ([&] (size_t j, Tx polsy) LAMBDA_INLINE
				  {				    
				    jac2.EvalMult(oi - k - j, 2 * ls - 1, polsy, 
						  SBLambda([&](size_t j, Tx val) LAMBDA_INLINE
							   {
							     shape[ii++] =  T_Id_v<3>(val);	      							     					     							   }));
				    jac2.IncAlpha2();
				  }));
		     jac1.IncAlpha2();
		   }));

      
      //############ type 2 ############
            
      leg.EvalScaled1Assign 
	(oi-1, lt-lo, lt+lo,
	 SBLambda ([&](size_t k, Tx polz) LAMBDA_INLINE
		   {
		     JacobiPolynomialAlpha jac2(2*k+2);
 
		     jac1.EvalScaledMult1Assign
		       (oi-1-k, le-lt-lo, 1-ls, polz, 
			SBLambda ([&] (size_t j, Tx polsy) LAMBDA_INLINE
				  {				    
				    jac2.EvalMult(oi-1 - k - j, 2 * ls - 1, polsy, 
						  SBLambda([&](size_t j, Tx val) LAMBDA_INLINE
							   {
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(le,ls,lt,lo*val);	      
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(ls,lt,le,lo*val);	  
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(le,ls,lo,lt*val);	      
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(ls,lo,le,lt*val);	  
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(le,lo,lt,ls*val);	      
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(lo,lt,le,ls*val);	  
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(lo,ls,lt,le*val);	      
							     shape[ii++] =  T_Dl1_o_Dl2xDl3_v(lt,ls,lo,le*val);							     
							   }));
				    jac2.IncAlpha2();
				  }));
		     jac1.IncAlpha2();
		   }));
     };
  };
  
  ////////////////////// SURFACE ////////////////////////////
    template <int DIM>
  class HCurlDivSurfaceFiniteElement : public FiniteElement
  {
  public:
    using FiniteElement::FiniteElement;
    using FiniteElement::ndof;
    using FiniteElement::order;

    virtual void CalcMappedShape (const MappedIntegrationPoint<DIM,DIM+1> & mip,
      BareSliceMatrix<double> shape) const = 0;

    virtual void CalcShape (const IntegrationPoint & ip, 
                            BareSliceMatrix<double> shape) const = 0;

  };


  template <ELEMENT_TYPE ET> class HCurlDivSurfaceFE;

  
  template <ELEMENT_TYPE ET>
  class T_HCurlDivSurfaceFE : public HCurlDivSurfaceFiniteElement<ET_trait<ET>::DIM>,
    public VertexOrientedFE<ET>
  {
  protected:
    enum { DIM = ET_trait<ET>::DIM };
    enum { DIM_STRESS = DIM }; //check this!!!
    
    using VertexOrientedFE<ET>::vnums;
    using HCurlDivSurfaceFiniteElement<ET_trait<ET>::DIM>::ndof;
    using HCurlDivSurfaceFiniteElement<ET_trait<ET>::DIM>::order;

    int order_inner;


  public:
    using VertexOrientedFE<ET>::SetVertexNumbers;
    
    T_HCurlDivSurfaceFE (int aorder)
    {
      order = aorder;
      order_inner = aorder;
    }
    
    virtual ELEMENT_TYPE ElementType() const { return ET; }
    const HCurlDivSurfaceFE<ET> * Cast() const { return static_cast<const HCurlDivSurfaceFE<ET>*> (this); } 
    
    INLINE void SetOrderInner (int order) { order_inner = order; }

    virtual void ComputeNDof()
    {
      cout << "Error, T_HCurlDivSurfaceFE<ET>:: ComputeNDof not available for base class" << endl;
    }

    virtual void CalcShape (const IntegrationPoint & ip, 
                            BareSliceMatrix<double> shape) const
    {
      Vec<DIM, AutoDiffDiff<DIM>> adp;
      for ( int i=0; i<DIM; i++)
      {
        adp(i) = AutoDiffDiff<DIM>(ip(i),i);
      }

      Cast() -> T_CalcShape (TIP<DIM, AutoDiffDiff<DIM>> (adp), SBLambda([&] (int nr, auto val)
                                          {
                                            //shape.Row(nr).AddSize(DIM_STRESS) = val.Shape();
					    shape.Row(nr).AddSize(DIM_STRESS) = val;
                                          }));
    }

    
    virtual void CalcMappedShape (const MappedIntegrationPoint<DIM,DIM+1> & mip,
                            BareSliceMatrix<double> shape) const
    {
      //Vec<DIM, AutoDiff<DIM+1>> adp = mip;
      //Vec<DIM, AutoDiffDiff<DIM+1>> addp;
      //for (int i=0; i<DIM+1; i++)
      //{
      //  addp[i] = adp[i].Value();
      //  addp[i].LoadGradient(&adp[i].DValue(0));
      //}
      throw Exception("not implemented");
      //Vec<DIM, AutoDiffDiff<DIM+1>> adp;
      //for ( int i=0; i<DIM+1; i++)
      //{
      //  adp(i) = AutoDiffDiff<DIM+1>(mip.IP()(i),i);
      //}
      //Cast() -> T_CalcShape (TIP<DIM,AutoDiffDiff<DIM+1>> (adp),SBLambda([&](int nr,auto val)
      //{
      //  Vec<DIM_STRESS> vecshape = val.Shape();
      //	//Vec<DIM_STRESS> vecshape = val;
      //  BareVector<double> matshape = shape.Row(nr);
      //  VecToMat<DIM+1> (vecshape, matshape);
      //}));
    }


  };

  template <> class HCurlDivSurfaceFE<ET_SEGM> : public T_HCurlDivSurfaceFE<ET_SEGM> 
  {
    
  public:
    using T_HCurlDivSurfaceFE<ET_SEGM> :: T_HCurlDivSurfaceFE;

    virtual void ComputeNDof()
    {
      order = 0;
      ndof = 0;
      ndof += order_inner+1;
      order = max2(order,order_inner);

    }
   template <typename Tx, typename TFA> 
    void T_CalcShape (TIP<1,Tx> ip/*AutoDiffDiff<2> hx[2]*/, TFA & shape) const
    {
      auto x = ip.x;
      AutoDiffDiff<1> ddlami[2] ={ x, 1-x };
      
      int ii = 0;
      
      ArrayMem<AutoDiffDiff<1>,20> ha(order_inner+1);
      
      int es = 0,ee = 1;
      if(vnums[es] > vnums[ee]) swap (es,ee);

      AutoDiffDiff<1> ls = ddlami[es],le = ddlami[ee];
      
      LegendrePolynomial::Eval(order_inner,le-ls,ha);      
      
      for(int l = 0; l <= order_inner; l++)
	shape[ii++] =  -1* ha[l].Value(); //I think there should be a minus
    };
  };


  
  /* Face basis functions which are normal-tangential continuous */
  /* calculates [(grad l1) o-times (grad l2 x grad l3)] * legendre */
  /* (grad l2 x grad l3) is a scalar!! (cross product on surface */
  
  class T_Dl1_o_Dl2xDl3_v_surf
  {
    AutoDiffDiff<2> l1,l2,l3,v;
  public:
    T_Dl1_o_Dl2xDl3_v_surf  (AutoDiffDiff<2> lam1, AutoDiffDiff<2> lam2, AutoDiffDiff<2> lam3, AutoDiffDiff<2> av) : l1(lam1), l2(lam2), l3(lam3), v(av) { ; }
    
    Vec<2> Shape() {
      double cross = l2.DValue(0)*l3.DValue(1) - l2.DValue(1)*l3.DValue(0);      
      return Vec<2> (v.Value()*l1.DValue(0) * cross,  v.Value()*l1.DValue(1) * cross);
    }

    Vec<2> DivShape()
    {
      throw Exception("not available on surface");
    }

  };
  
  template <> class HCurlDivSurfaceFE<ET_TRIG> : public T_HCurlDivSurfaceFE<ET_TRIG> 
  {
    
  public:
    using T_HCurlDivSurfaceFE<ET_TRIG> :: T_HCurlDivSurfaceFE;

    virtual void ComputeNDof()
    {
      order = 0;
      ndof = 0;
      ndof += (order_inner+1) * (order_inner+2);
      order = max2(order,order_inner);
    }
    
   template <typename Tx, typename TFA> 
    void T_CalcShape (TIP<2,Tx> ip/*AutoDiffDiff<2> hx[2]*/, TFA & shape) const
    {
     
      auto x = ip.x, y= ip.y;
      AutoDiffDiff<2> ddlami[3] ={ x, y, 1-x-y };
      
      int ii = 0;

      ArrayMem<AutoDiffDiff<2>,20> ha((order_inner+1)*(order_inner+2)/2.0); //face basis
      
      int es = 0, ee = 1, et = 2;
      if(vnums[es] > vnums[ee]) swap(es, ee);
      if(vnums[ee] > vnums[et]) swap(ee, et);
      if(vnums[es] > vnums[et]) swap(es,et);
            
      AutoDiffDiff<2> ls = ddlami[es],le = ddlami[ee], lt = ddlami[et];
      
      DubinerBasis3::Eval (order_inner, ls, le, ha);

      for (int l = 0; l < (order_inner+1)*(order_inner+2)/2.0; l++)
	    {
	      shape[ii++] = T_Dl1_o_Dl2xDl3_v_surf(le,ls,lt,ha[l]).Shape();
	      shape[ii++] = T_Dl1_o_Dl2xDl3_v_surf(ls,lt,le,ha[l]).Shape();
	    }     
    };
  };

}


#endif
  
