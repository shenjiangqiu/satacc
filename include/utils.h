//
// Created by Jiangqiu shen on 4/15/21.
//

#ifndef MY_SATACC_PROJECT_UTILS_H
#define MY_SATACC_PROJECT_UTILS_H

template <typename T, typename Q,typename M>
void update(T &average, const Q &current, const M &current_cycle) {
  average+=(current-average)/(double)(current_cycle+1);

}

#endif // MY_SATACC_PROJECT_UTILS_H
